"""Offline tests for cxsession, against a fake camera backend.

The fake runs a real local `sh` behind the same socket interface PMCA's
backend uses, and serves "partitions" from a temp directory, so the session
logic (output capture, hashing, verification, manifest) is exercised for real.
"""

import hashlib
import json
import os
import select
import subprocess
import sys
import tempfile
import unittest

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import cxsession


class FakeBackend(object):
    """Stands in for pmca.platform.backend.usb.UsbPlatformBackend."""

    def __init__(self, root, md5sum=True, flaky=()):
        self.root = root          # directory playing the camera's /
        self.md5sum = md5sum      # does the camera have an md5sum applet?
        self.flaky = set(flaky)   # paths whose content changes between reads
        self.reads = {}

    def _local(self, path):
        return os.path.join(self.root, path.lstrip('/'))

    def interactiveShell(self, conn):
        env = dict(os.environ, CXROOT=self.root)
        # `cat /proc/x` and `md5sum /dev/x` are redirected into the fake root.
        prelude = 'cat() { command cat "$CXROOT$1"; }\n'
        if self.md5sum:
            prelude += 'md5sum() { command md5sum "$CXROOT$1"; }\n'
        else:
            prelude += 'md5sum() { echo "md5sum: applet not found"; return 127; }\n'
        p = subprocess.Popen(['sh'], stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, env=env)
        p.stdin.write(prelude.encode())
        p.stdin.flush()
        while p.poll() is None:
            r, _, _ = select.select([conn, p.stdout], [], [], 0.05)
            if conn in r:
                d = conn.recv(65536)
                if d:
                    p.stdin.write(d)
                    p.stdin.flush()
            if p.stdout in r:
                d = os.read(p.stdout.fileno(), 65536)
                if d:
                    conn.sendall(d)
        rest = p.stdout.read()
        if rest:
            conn.sendall(rest)
        conn.close()

    def readFile(self, path, f, sizeCb=None):
        with open(self._local(path), 'rb') as src:
            data = src.read()
        n = self.reads[path] = self.reads.get(path, 0) + 1
        if path in self.flaky and n > 1:
            data = data[:-1] + bytes([data[-1] ^ 1])
        if sizeCb:
            sizeCb(len(data))
        for i in range(0, len(data), 0xfff8):
            f.write(data[i:i + 0xfff8])

    def getBackupData(self):
        return b'BACKUP' * 100

    def getBackupStatus(self):
        return b'\0' * 0x14 + b'PJ_U2\0'

    def readBootloader(self):
        return iter([b'boot-safe' * 10, b'boot-normal' * 10])

    def readMemory(self, offset, size, f):
        f.write(b'\xea' * size)


def make_camera(tmp):
    os.makedirs(os.path.join(tmp, 'proc'))
    os.makedirs(os.path.join(tmp, 'dev'))
    # Kernel names as the HDR-CX580V reports them in /proc/partitions.
    sizes = {'loop0': 12, 'nflasha0': 3072, 'nflasha0p1': 1024, 'nflasha0p2': 2048, 'nflashb0': 4 * 1024 * 1024}
    nodes = {'loop0': 'loop0', 'nflasha0': 'nflasha', 'nflasha0p1': 'nflasha1', 'nflasha0p2': 'nflasha2'}
    with open(os.path.join(tmp, 'proc', 'partitions'), 'w') as f:
        f.write('major minor  #blocks  name\n\n')
        for i, (name, kb) in enumerate(sizes.items()):
            f.write(' 254 %8d %10d %s\n' % (i, kb, name))
    for name, node in nodes.items():
        with open(os.path.join(tmp, 'dev', node), 'wb') as f:
            f.write(os.urandom(sizes[name] * 1024))
    return sizes


class SessionTest(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.cam = os.path.join(self.tmp.name, 'cam')
        self.out = os.path.join(self.tmp.name, 'out')
        os.makedirs(self.cam)
        self.sizes = make_camera(self.cam)

    def tearDown(self):
        self.tmp.cleanup()

    def session(self, **kw):
        return cxsession.Session(FakeBackend(self.cam, **kw), log=lambda *a: None)

    def test_sh_captures_output_and_status_per_command(self):
        r = self.session().sh_many(['echo hello', 'false', 'echo a; echo b'])
        self.assertEqual([(s, o) for _, s, o in r], [(0, 'hello\n'), (1, ''), (0, 'a\nb\n')])

    def test_sh_captures_stderr(self):
        status, out = self.session().sh('ls /definitely/not/here')
        self.assertNotEqual(status, 0)
        self.assertIn('No such file', out)

    def test_parse_partitions(self):
        self.assertEqual(
            cxsession.parse_partitions(open(os.path.join(self.cam, 'proc', 'partitions')).read()),
            [(n, kb * 1024) for n, kb in self.sizes.items()])

    def test_device_node_follows_the_sony_nflash_naming(self):
        # Seen on the HDR-CX580V: /proc/partitions says nflasha0p2 (252,2)
        # while the node mounted at /setting is /dev/nflasha2 (252,2).
        self.assertEqual(cxsession.device_node('nflasha0'), '/dev/nflasha')
        self.assertEqual(cxsession.device_node('nflasha0p2'), '/dev/nflasha2')
        self.assertEqual(cxsession.device_node('nflasha0p15'), '/dev/nflasha15')
        self.assertEqual(cxsession.device_node('nflashb0p1'), '/dev/nflashb1')
        self.assertEqual(cxsession.device_node('loop0'), '/dev/loop0')

    def test_dump_verifies_with_md5_on_camera(self):
        m = cxsession.dump(self.session(), self.out)
        self.assertEqual([p['file'] for p in m['partitions']], ['nflasha1.bin', 'nflasha2.bin', 'nflasha.bin'])
        for p in m['partitions']:
            self.assertEqual(p['verified'], 'md5-on-camera')
            with open(os.path.join(self.cam, 'dev', p['file'][:-4]), 'rb') as f:
                self.assertEqual(p['sha256'], hashlib.sha256(f.read()).hexdigest())
        self.assertEqual([(s['name'], s['reason']) for s in m['skipped']], [
            ('loop0', 'not a flash device'),
            ('nflashb0', 'larger than firmware partition limit')])
        self.assertEqual([p['remote'] for p in m['partitions']],
                         ['/dev/nflasha1', '/dev/nflasha2', '/dev/nflasha'])
        self.assertEqual(sorted(e['file'] for e in m['extra']),
                         ['Backup.bin', 'backup_status.bin', 'boot1.bin', 'boot2.bin', 'bootrom.bin'])
        self.assertEqual(cxsession.verify_manifest(self.out), [])
        self.assertTrue(os.path.getsize(os.path.join(self.out, 'recon.txt')) > 0)

    def test_dump_falls_back_to_double_read_without_md5sum(self):
        m = cxsession.dump(self.session(md5sum=False), self.out)
        self.assertEqual({p['verified'] for p in m['partitions']}, {'double-read'})
        self.assertFalse([f for f in os.listdir(self.out) if f.endswith('.verify')])

    def test_dump_refuses_a_partition_that_reads_differently_twice(self):
        with self.assertRaises(cxsession.VerifyError):
            cxsession.dump(self.session(md5sum=False, flaky=['/dev/nflasha2']), self.out)
        self.assertFalse(os.path.exists(os.path.join(self.out, 'manifest.json')))
        # What did verify before the failure is still on record.
        with open(os.path.join(self.out, 'manifest.partial.json')) as f:
            self.assertEqual([p['file'] for p in json.load(f)['partitions']], ['nflasha1.bin'])

    def test_dump_refuses_a_copy_that_differs_from_the_camera_md5(self):
        s = self.session()
        s.remote_md5 = lambda remote: '0' * 32
        with self.assertRaises(cxsession.VerifyError):
            cxsession.dump(s, self.out)

    def test_verify_manifest_detects_corruption_on_disk(self):
        cxsession.dump(self.session(), self.out)
        with open(os.path.join(self.out, 'nflasha1.bin'), 'r+b') as f:
            f.write(b'\xff\xfe')
        os.remove(os.path.join(self.out, 'boot2.bin'))
        self.assertEqual(sorted(cxsession.verify_manifest(self.out)), ['boot2.bin', 'nflasha1.bin'])


class VerifyManifestTest(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)
        self.out = self.tmp.name
        self.data = b'backup'
        self.entry = {'file': 'backup.bin', 'size': len(self.data),
                      'sha256': hashlib.sha256(self.data).hexdigest()}
        with open(os.path.join(self.out, self.entry['file']), 'wb') as f:
            f.write(self.data)

    def write_manifest(self, entries):
        with open(os.path.join(self.out, 'manifest.json'), 'w', encoding='utf-8') as f:
            json.dump({'partitions': entries, 'extra': []}, f)

    def test_size_must_match_even_with_correct_digest(self):
        self.write_manifest([dict(self.entry, size=len(self.data) + 1)])
        self.assertEqual(cxsession.verify_manifest(self.out), ['backup.bin'])

    def test_rejects_empty_and_invalid_manifests(self):
        for entries in ([], [self.entry, self.entry], [dict(self.entry, file='../backup.bin')],
                        [dict(self.entry, size=True)], [dict(self.entry, sha256='invalid')]):
            with self.subTest(entries=entries):
                self.write_manifest(entries)
                with self.assertRaises(cxsession.VerifyError):
                    cxsession.verify_manifest(self.out)

    def test_rejects_symlink_to_matching_file(self):
        os.symlink('backup.bin', os.path.join(self.out, 'linked.bin'))
        self.write_manifest([dict(self.entry, file='linked.bin')])
        self.assertEqual(cxsession.verify_manifest(self.out), ['linked.bin'])


if __name__ == '__main__':
    unittest.main()
