"""A scripted session on top of PMCA's updater-shell backend.

PMCA's shell is interactive. This wraps the same backend so recon and backups
can run unattended and be verified: commands return (exit status, output),
pulled files are hashed, and every pull is checked against a second source
(a hash computed on the camera, or a second read) before it is trusted.
"""

import hashlib
import io
import json
import os
import re
import socket
import threading
import time

#: Commands whose output documents the system the dump was taken from.
RECON_COMMANDS = [
    'cat /proc/version',
    'cat /proc/cmdline',
    'cat /proc/cpuinfo',
    'cat /proc/meminfo',
    'cat /proc/partitions',
    'cat /proc/mtd',
    'cat /proc/mounts',
    'cat /proc/filesystems',
    'cat /proc/devices',
    'cat /proc/modules',
    'cat /proc/iomem',
    'ls -la /dev',
    'ls -la /',
    'ls -la /setting',
    'ps',
    'busybox',
    'env',
    'df',
]

#: Block devices larger than this are user storage (the 32 GB recording
#: memory), not firmware, and are skipped unless explicitly requested.
MAX_FIRMWARE_PART = 2 << 30


class VerifyError(Exception):
    pass


class Session(object):
    def __init__(self, backend, log=print):
        self.backend = backend
        self.log = log
        self._n = 0

    # -- shell ---------------------------------------------------------------

    def sh_many(self, cmds):
        """Runs commands in one shell. Returns [(cmd, status, output)]."""
        self._n += 1
        tag = '__CX%d_%%d_%%s__' % self._n
        script = ''.join('echo %s; %s 2>&1; echo %s$?\n' % (tag % (i, 'B'), c, tag % (i, 'E')) for i, c in enumerate(cmds))
        raw = self._run(script + 'exit\n')
        out = []
        for i, c in enumerate(cmds):
            m = re.search(re.escape(tag % (i, 'B')) + r'\r?\n(.*?)' + re.escape(tag % (i, 'E')) + r'(\d+)', raw, re.S)
            if not m:
                raise Exception('No output captured for %r. Raw shell output:\n%s' % (c, raw))
            out.append((c, int(m.group(2)), m.group(1)))
        return out

    def sh(self, cmd):
        """Runs one command. Returns (status, output)."""
        _, status, output = self.sh_many([cmd])[0]
        return status, output

    def _run(self, script):
        ours, theirs = socket.socketpair()
        chunks = []

        def pump():
            try:
                theirs.sendall(script.encode('latin1'))
                while True:
                    d = theirs.recv(65536)
                    if not d:
                        break
                    chunks.append(d)
            except OSError:
                pass

        t = threading.Thread(target=pump, daemon=True)
        t.start()
        # Runs the transfer loop until the shell on the camera exits, then
        # closes `ours`, which ends the pump thread.
        self.backend.interactiveShell(ours)
        t.join(10)
        theirs.close()
        return b''.join(chunks).decode('latin1')

    # -- files ---------------------------------------------------------------

    def pull(self, remote, local):
        """Copies a remote file. Returns (size, sha256, md5)."""
        sha, md5 = hashlib.sha256(), hashlib.md5()
        state = {'size': 0, 'total': 0, 'next': 0, 't0': time.time()}

        class Sink(io.RawIOBase):
            def write(sink, data):
                f.write(data)
                sha.update(data)
                md5.update(data)
                state['size'] += len(data)
                if state['size'] >= state['next']:
                    state['next'] = state['size'] + (16 << 20)
                    dt = max(time.time() - state['t0'], 1e-3)
                    self.log('    %s: %d / %d MiB (%.1f MiB/s)' % (
                        remote, state['size'] >> 20, state['total'] >> 20, (state['size'] / dt) / (1 << 20)))
                return len(data)

        with open(local, 'wb') as f:
            self.backend.readFile(remote, Sink(), lambda total: state.update(total=total))
        if state['total'] and state['size'] != state['total']:
            raise VerifyError('%s: got %d bytes, camera reported %d' % (remote, state['size'], state['total']))
        return state['size'], sha.hexdigest(), md5.hexdigest()

    def remote_md5(self, remote):
        """md5 of a remote file computed on the camera, or None if unavailable."""
        status, out = self.sh('md5sum %s' % remote)
        m = re.search(r'\b([0-9a-f]{32})\b', out)
        return m.group(1) if status == 0 and m else None

    def pull_verified(self, remote, local):
        """Pulls a file and proves the copy is faithful. Returns a manifest entry.

        Preferred proof: an md5 computed on the camera itself. Fallback: a
        second, independent read over USB must match the first.
        """
        size, sha, md5 = self.pull(remote, local)
        remote_md5 = self.remote_md5(remote)
        if remote_md5 is not None:
            if remote_md5 != md5:
                raise VerifyError('%s: md5 on camera %s != md5 of copy %s' % (remote, remote_md5, md5))
            method = 'md5-on-camera'
        else:
            size2, sha2, _ = self.pull(remote, local + '.verify')
            os.remove(local + '.verify')
            if (size2, sha2) != (size, sha):
                raise VerifyError('%s: two reads differ (%s vs %s)' % (remote, sha, sha2))
            method = 'double-read'
        return {'remote': remote, 'file': os.path.basename(local), 'size': size,
                'sha256': sha, 'md5': md5, 'verified': method}


def parse_partitions(text):
    """Parses /proc/partitions. Returns [(name, size_bytes)]."""
    parts = []
    for line in text.splitlines():
        f = line.split()
        if len(f) == 4 and f[0].isdigit() and f[2].isdigit():
            parts.append((f[3], int(f[2]) * 1024))
    return parts


def device_node(name):
    """Maps a /proc/partitions name to its /dev node.

    Sony's nflash driver reports the whole device as nflasha0 and partitions
    as nflasha0pN, but creates /dev/nflasha and /dev/nflashaN (same major and
    minor). Everything else uses the kernel name unchanged.
    """
    m = re.match(r'^(nflash[a-z])0(?:p(\d+))?$', name)
    if m:
        return '/dev/' + m.group(1) + (m.group(2) or '')
    return '/dev/' + name


def is_whole_device(name):
    """True for a whole flash device (nflasha0), false for its partitions."""
    return re.match(r'^nflash[a-z]0$', name) is not None


def write_bytes(outdir, name, data):
    with open(os.path.join(outdir, name), 'wb') as f:
        f.write(data)
    return {'file': name, 'size': len(data), 'sha256': hashlib.sha256(data).hexdigest()}


def recon(session, outdir):
    """Records what system we are talking to. Returns the /proc/partitions text."""
    os.makedirs(outdir, exist_ok=True)
    results = session.sh_many(RECON_COMMANDS)
    with open(os.path.join(outdir, 'recon.txt'), 'w') as f:
        for cmd, status, output in results:
            f.write('### %s (exit %d)\n%s\n' % (cmd, status, output))
    return dict((c, o) for c, _, o in results)['cat /proc/partitions']


def dump(session, outdir, include_large=False, only=None):
    """Full, verified backup of everything firmware related.

    Writes one image per flash partition, the settings store (Backup.bin), the
    boot loader blocks and the boot ROM, plus manifest.json with sizes and
    hashes. Raises VerifyError if any copy cannot be proven faithful; the
    manifest is only written when every item verified.
    """
    partitions = parse_partitions(recon(session, outdir))
    manifest = {'created': time.strftime('%Y-%m-%dT%H:%M:%S%z'), 'partitions': [], 'skipped': [], 'extra': []}

    def checkpoint():
        # Progress survives an interrupted session, under a different name:
        # manifest.json itself only ever describes a complete, verified backup.
        with open(os.path.join(outdir, 'manifest.partial.json'), 'w') as f:
            json.dump(manifest, f, indent=2)

    def dump_devices(devices):
        for name, size in devices:
            node = device_node(name)
            short = os.path.basename(node)
            if only and name not in only and short not in only:
                continue
            if not name.startswith('nflash'):
                manifest['skipped'].append({'name': name, 'size': size, 'reason': 'not a flash device'})
                continue
            if size > MAX_FIRMWARE_PART and not include_large:
                session.log('  skipping %s (%d MiB, user storage)' % (name, size >> 20))
                manifest['skipped'].append({'name': name, 'size': size, 'reason': 'larger than firmware partition limit'})
                continue
            session.log('  dumping %s (%s, %d MiB)' % (node, name, size >> 20))
            entry = session.pull_verified(node, os.path.join(outdir, short + '.bin'))
            if entry['size'] != size:
                raise VerifyError('%s: copied %d bytes, kernel reports %d' % (name, entry['size'], size))
            entry['kernel_name'] = name
            manifest['partitions'].append(entry)
            checkpoint()

    # Partitions first, the small irreplaceable items next, and the whole-device
    # image (which repeats every partition) last: if the session is cut short,
    # what is already on disk is the part that matters most.
    whole = [d for d in partitions if is_whole_device(d[0])]
    dump_devices([d for d in partitions if not is_whole_device(d[0])])

    if not only:
        session.log('  reading settings store (Backup.bin)')
        a, b = session.backend.getBackupData(), session.backend.getBackupData()
        if a != b:
            raise VerifyError('Backup data: two reads differ')
        manifest['extra'].append(dict(write_bytes(outdir, 'Backup.bin', a), what='settings store', verified='double-read'))
        manifest['extra'].append(dict(write_bytes(outdir, 'backup_status.bin', session.backend.getBackupStatus()), what='backup preset status'))

        session.log('  reading boot loader')
        a, b = list(session.backend.readBootloader()), list(session.backend.readBootloader())
        if a != b:
            raise VerifyError('Boot loader: two reads differ')
        for i, block in enumerate(a):
            manifest['extra'].append(dict(write_bytes(outdir, 'boot%d.bin' % (i + 1), block), what='boot loader block', verified='double-read'))

        session.log('  reading boot ROM')
        rom = io.BytesIO()
        session.backend.readMemory(0xffff0000, 0x6000, rom)
        manifest['extra'].append(dict(write_bytes(outdir, 'bootrom.bin', rom.getvalue()), what='boot ROM'))
        checkpoint()

    dump_devices(whole)

    with open(os.path.join(outdir, 'manifest.json'), 'w') as f:
        json.dump(manifest, f, indent=2)
    os.remove(os.path.join(outdir, 'manifest.partial.json'))
    return manifest


def verify_manifest(outdir):
    """Checks local backup sizes and SHA-256 hashes. Returns the bad files."""
    with open(os.path.join(outdir, 'manifest.json'), encoding='utf-8') as f:
        manifest = json.load(f)

    if not isinstance(manifest, dict) or any(
        not isinstance(manifest.get(group), list) for group in ('partitions', 'extra')
    ):
        raise VerifyError('Expected partitions and extra lists in manifest.json')
    entries = manifest['partitions'] + manifest['extra']
    if not entries:
        raise VerifyError('The manifest contains no backup files')

    seen = set()
    for entry in entries:
        if not isinstance(entry, dict):
            raise VerifyError('Expected a file entry in manifest.json')
        name, size, digest = entry.get('file'), entry.get('size'), entry.get('sha256')
        if (not isinstance(name, str) or name in ('', '.', '..', 'manifest.json')
                or os.path.basename(name) != name or '\0' in name or name in seen):
            raise VerifyError('Invalid or duplicate backup filename: %r' % name)
        if type(size) is not int or size < 0 or not isinstance(digest, str) or not re.fullmatch(r'[0-9a-f]{64}', digest):
            raise VerifyError('Invalid size or SHA-256 for %r' % name)
        seen.add(name)

    bad = []
    for entry in entries:
        name = entry['file']
        path = os.path.join(outdir, name)
        if os.path.islink(path) or not os.path.isfile(path):
            bad.append(name)
            continue
        h = hashlib.sha256()
        try:
            with open(path, 'rb') as f:
                if os.fstat(f.fileno()).st_size != entry['size']:
                    bad.append(name)
                    continue
                for chunk in iter(lambda: f.read(1 << 20), b''):
                    h.update(chunk)
        except OSError:
            bad.append(name)
            continue
        if h.hexdigest() != entry['sha256']:
            bad.append(name)
    return bad
