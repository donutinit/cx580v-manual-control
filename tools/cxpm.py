"""La partición de PlayMemories (nflasha12) desde el modo updater.

En modo normal el init de la cámara la monta en /log (FAT16 a partir del sector
3, por un dispositivo loop) y ahí busca cxg.so el programa de DOOM y su WAD. En
modo updater no está montada y el busybox no trae losetup, así que se sube
tools/bin/cxloop a la memoria (/ramdsk), se asocia /dev/loop1 con el
desplazamiento y se monta con el mount de siempre.

Todo lo que escribe pasa por install() y remove(), que antes sacan una copia
verificada de la partición entera. check() sólo lee.
"""
import io
import json
import os
import time

import cxguard

DEVICE = '/dev/nflasha12'
LOOP = '/dev/loop1'
OFFSET = 1536                 # el FAT16 empieza en el sector 3
MOUNT = '/ramdsk/pm'
HELPER = '/ramdsk/cxloop'
MARKER = 'PMHOME.EXE'         # si no está, no es la partición que creemos
MARGIN = 512 * 1024           # espacio que tiene que sobrar después de copiar
HELPER_PATH = os.path.join(os.path.dirname(os.path.abspath(__file__)), 'bin', 'cxloop')


class PmError(Exception):
    pass


def _sh(session, cmd, what):
    status, out = session.sh(cmd)
    if status != 0:
        raise PmError('%s failed (%d): %s\n%s' % (what, status, cmd, out.strip()))
    return out


def _valid_name(name):
    base, _, ext = name.partition('.')
    ok = lambda s: s and s.isalnum() and s.upper() == s
    return ok(base) and len(base) <= 8 and (not ext or (ok(ext) and len(ext) <= 3))


def load_helper():
    with open(HELPER_PATH, 'rb') as f:
        return f.read()


def detach(session):
    """Unmounts and frees the loop device. Safe to call when nothing is attached."""
    session.sh('sync')
    session.sh('umount %s' % MOUNT)
    session.sh('%s detach %s' % (HELPER, LOOP))


def attach(session, helper, writable):
    """Mounts the PlayMemories FAT at MOUNT. Returns a CameraFS for it."""
    fs = cxguard.CameraFS(session)
    if not fs.exists(HELPER) or fs.md5(HELPER) != cxguard.md5(helper):
        fs.push(HELPER, helper)
        if fs.md5(HELPER) != cxguard.md5(helper):
            raise PmError('the loop helper did not arrive intact at %s' % HELPER)
    _sh(session, 'chmod 755 %s' % HELPER, 'chmod')
    out = _sh(session, '%s attach %s %s %d %s' % (HELPER, LOOP, DEVICE, OFFSET, 'rw' if writable else 'ro'), 'loop attach')
    if 'ok asociado' not in out:
        raise PmError('unexpected answer from the loop helper: %s' % out.strip())
    try:
        _sh(session, 'mkdir -p %s' % MOUNT, 'mkdir')
        _sh(session, 'mount -t vfat -o %s %s %s' % ('rw,noatime' if writable else 'ro', LOOP, MOUNT), 'mount')
        if not fs.exists('%s/%s' % (MOUNT, MARKER)):
            raise PmError('%s is not on the mounted partition: this is not PlayMemories' % MARKER)
    except BaseException:
        detach(session)
        raise
    return fs


def _listing(session, fs):
    out = _sh(session, 'ls -l %s' % MOUNT, 'ls')
    return {'listing': out.strip(), 'free': fs.free_bytes(MOUNT)}


def check(session, helper):
    """Read-only: mounts, lists and unmounts. Returns {'listing', 'free'}."""
    fs = attach(session, helper, writable=False)
    try:
        return _listing(session, fs)
    finally:
        detach(session)


def snapshot(session, outdir):
    """Verified copy of the whole partition before changing it."""
    os.makedirs(outdir, exist_ok=True)
    session.sh('sync')
    entry = session.pull_verified(DEVICE, os.path.join(outdir, 'nflasha12.bin'))
    with open(os.path.join(outdir, 'manifest.json'), 'w') as f:
        json.dump({'created': time.strftime('%Y-%m-%dT%H:%M:%S%z'), 'partitions': [entry], 'extra': []}, f, indent=2)
    return entry


def _same(fs, name, data):
    path = '%s/%s' % (MOUNT, name)
    return fs.exists(path) and fs.md5(path) == cxguard.md5(data)


def install(session, helper, files, outdir, log=print):
    """Copies files ({NAME: bytes}, 8.3 upper-case names) to the partition.

    Order: read-only look, verified snapshot of the partition, copy, and a
    second mount to prove what was written survives unmounting.
    """
    for name in files:
        if not _valid_name(name) or name == MARKER:
            raise PmError('%r is not a plain 8.3 upper-case name' % name)
    fs = attach(session, helper, writable=False)
    try:
        before = _listing(session, fs)
        todo = dict((n, d) for n, d in files.items() if not _same(fs, n, d))
    finally:
        detach(session)
    log('PlayMemories partition before:\n%s\nfree %d KiB' % (before['listing'], before['free'] // 1024))
    if not todo:
        log('The files are already there and identical; nothing to write.')
        return before
    need = sum(len(d) for d in todo.values()) + MARGIN
    if before['free'] < need:
        raise PmError('not enough room: %d bytes free, %d needed' % (before['free'], need))
    entry = snapshot(session, outdir)
    log('Snapshot of %s written to %s (md5 %s)' % (DEVICE, outdir, entry['md5']))
    fs = attach(session, helper, writable=True)
    try:
        for name, data in sorted(todo.items()):
            path = '%s/%s' % (MOUNT, name)
            fs.push(path, data)
            if fs.md5(path) != cxguard.md5(data):
                fs.remove(path)
                raise PmError('%s: md5 on camera differs after copying; removed it' % name)
            log('  wrote %s, %d bytes' % (name, len(data)))
    finally:
        detach(session)
    fs = attach(session, helper, writable=False)
    try:
        for name, data in sorted(files.items()):
            if not _same(fs, name, data):
                raise PmError('%s does not read back the same after remounting' % name)
        after = _listing(session, fs)
    finally:
        detach(session)
    log('PlayMemories partition after:\n%s\nfree %d KiB' % (after['listing'], after['free'] // 1024))
    return after


def remove(session, helper, names, outdir, log=print):
    """Deletes our files from the partition (after a verified snapshot)."""
    for name in names:
        if not _valid_name(name) or name == MARKER:
            raise PmError('%r is not one of our files' % name)
    entry = snapshot(session, outdir)
    log('Snapshot of %s written to %s (md5 %s)' % (DEVICE, outdir, entry['md5']))
    fs = attach(session, helper, writable=True)
    try:
        for name in names:
            fs.remove('%s/%s' % (MOUNT, name))
        after = _listing(session, fs)
    finally:
        detach(session)
    log('PlayMemories partition after:\n%s\nfree %d KiB' % (after['listing'], after['free'] // 1024))
    return after
