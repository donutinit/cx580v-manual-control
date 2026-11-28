import hashlib
import os
import shutil
import tempfile
import unittest

import cxpm


class FakeBackend(object):
    def __init__(self, cam):
        self.cam = cam

    def writeFile(self, path, f):
        data = f.read()
        if path.startswith(cxpm.MOUNT + '/'):
            assert self.cam.mounted == 'rw', 'write while not mounted read-write'
            name = path[len(cxpm.MOUNT) + 1:]
            if self.cam.corrupt == name:
                data = data[:-1] + b'\0'
            self.cam.pm[name] = data
            self.cam.writes.append(name)
        else:
            self.cam.ram[path] = data


class FakeCamera(object):
    """The updater shell, as far as cxpm uses it."""

    def __init__(self, free=8617984, marker=True):
        self.ram = {}
        self.pm = {'PMHOME.EXE': b'x' * 100} if marker else {'OTHER.BIN': b'y'}
        self.free = free
        self.attached = None
        self.mounted = None
        self.corrupt = None
        self.writes = []
        self.cmds = []
        self.backend = FakeBackend(self)
        self.snapshots = 0

    def _file(self, path):
        if path.startswith(cxpm.MOUNT + '/'):
            return self.pm.get(path[len(cxpm.MOUNT) + 1:]) if self.mounted else None
        return self.ram.get(path)

    def sh(self, cmd):
        self.cmds.append(cmd)
        w = cmd.split()
        if w[0] == 'test':
            return (0 if self._file(w[2]) is not None else 1), ''
        if w[0] == 'md5sum':
            d = self._file(w[1])
            return (0, '%s  %s\n' % (hashlib.md5(d).hexdigest(), w[1])) if d is not None else (1, 'no such file')
        if w[0] == cxpm.HELPER and w[1] == 'attach':
            assert self.attached is None and w[2:5] == [cxpm.LOOP, cxpm.DEVICE, '1536']
            self.attached = w[5]
            return 0, 'ok asociado, desplazamiento 1536 %s\n' % w[5]
        if w[0] == cxpm.HELPER and w[1] == 'detach':
            self.attached = None
            return 0, 'ok suelto\n'
        if w[0] == 'mount':
            mode = 'rw' if w[4].startswith('rw') else 'ro'
            assert self.attached is not None and not (mode == 'rw' and self.attached == 'ro')
            self.mounted = mode
            return 0, ''
        if w[0] == 'umount':
            self.mounted = None
            return 0, ''
        if w[0] == 'ls':
            return 0, '\n'.join('%8d %s' % (len(d), n) for n, d in sorted(self.pm.items()))
        if w[0] == 'df':
            used = sum(len(d) for n, d in self.pm.items() if n not in ('PMHOME.EXE', 'OTHER.BIN'))
            return 0, 'Filesystem 1K-blocks Used Available Use%% Mounted on\n/dev/loop1 25000 1 %d 1%% %s\n' % (
                (self.free - used) // 1024, cxpm.MOUNT)
        if w[0] == 'rm':
            assert self.mounted == 'rw'
            self.pm.pop(w[2][len(cxpm.MOUNT) + 1:], None)
            return 0, ''
        return 0, ''

    def remote_md5(self, remote):
        status, out = self.sh('md5sum %s' % remote)
        return out.split()[0] if status == 0 else None

    def pull_verified(self, remote, local):
        assert self.mounted is None, 'snapshot must be taken with the partition unmounted'
        self.snapshots += 1
        self.cmds.append('SNAPSHOT')
        with open(local, 'wb') as f:
            f.write(b'image')
        return {'remote': remote, 'file': os.path.basename(local), 'size': 5, 'sha256': 's', 'md5': 'm', 'verified': 'md5-on-camera'}


class PmTest(unittest.TestCase):
    def setUp(self):
        self.out = tempfile.mkdtemp()
        self.addCleanup(shutil.rmtree, self.out)
        self.log = []

    def install(self, cam, files):
        return cxpm.install(cam, b'helper', files, os.path.join(self.out, 'snap'), log=self.log.append)

    def test_check_only_reads(self):
        cam = FakeCamera()
        info = cxpm.check(cam, b'helper')
        self.assertIn('PMHOME.EXE', info['listing'])
        self.assertEqual(info['free'], 8617984 // 1024 * 1024)
        self.assertEqual(cam.writes, [])
        self.assertIsNone(cam.mounted)
        self.assertIsNone(cam.attached)
        self.assertFalse(any('rw' in c for c in cam.cmds))

    def test_install_snapshots_first_then_writes_and_rereads(self):
        cam = FakeCamera()
        self.install(cam, {'CXDOOM.BIN': b'blob', 'DOOM1.WAD': b'wad' * 1000})
        self.assertEqual(cam.snapshots, 1)
        self.assertEqual(cam.pm['CXDOOM.BIN'], b'blob')
        self.assertEqual(sorted(cam.writes), ['CXDOOM.BIN', 'DOOM1.WAD'])
        self.assertTrue(os.path.exists(os.path.join(self.out, 'snap', 'manifest.json')))
        self.assertIsNone(cam.mounted)
        self.assertIsNone(cam.attached)
        # the snapshot comes before the first read-write mount
        first_rw = next(i for i, c in enumerate(cam.cmds) if c.endswith(' rw') or 'rw,' in c)
        self.assertLess(cam.cmds.index('SNAPSHOT'), first_rw)

    def test_identical_files_are_not_written_again(self):
        cam = FakeCamera()
        files = {'CXDOOM.BIN': b'blob', 'DOOM1.WAD': b'wad'}
        self.install(cam, files)
        cam.writes[:] = []
        self.install(cam, files)
        self.assertEqual(cam.writes, [])
        self.assertEqual(cam.snapshots, 1)
        self.install(cam, {'CXDOOM.BIN': b'blob v2', 'DOOM1.WAD': b'wad'})
        self.assertEqual(cam.writes, ['CXDOOM.BIN'])

    def test_refuses_without_room_before_touching_anything(self):
        cam = FakeCamera(free=1024 * 1024)
        with self.assertRaises(cxpm.PmError):
            self.install(cam, {'DOOM1.WAD': b'w' * (900 * 1024)})
        self.assertEqual(cam.writes, [])
        self.assertEqual(cam.snapshots, 0)

    def test_refuses_another_partition(self):
        cam = FakeCamera(marker=False)
        with self.assertRaises(cxpm.PmError):
            cxpm.check(cam, b'helper')
        self.assertIsNone(cam.mounted)
        self.assertIsNone(cam.attached)

    def test_bad_copy_is_removed(self):
        cam = FakeCamera()
        cam.corrupt = 'DOOM1.WAD'
        with self.assertRaises(cxpm.PmError):
            self.install(cam, {'DOOM1.WAD': b'wad' * 10})
        self.assertNotIn('DOOM1.WAD', cam.pm)
        self.assertIsNone(cam.mounted)

    def test_names_must_be_ours(self):
        cam = FakeCamera()
        for bad in ('PMHOME.EXE', 'doom1.wad', '../X', 'LONGFILENAME.BIN', 'A B.C'):
            with self.assertRaises(cxpm.PmError):
                self.install(cam, {bad: b'x'})
        self.assertEqual(cam.cmds, [])

    def test_remove(self):
        cam = FakeCamera()
        self.install(cam, {'CXDOOM.BIN': b'blob'})
        cxpm.remove(cam, b'helper', ['CXDOOM.BIN'], os.path.join(self.out, 'snap2'), log=self.log.append)
        self.assertNotIn('CXDOOM.BIN', cam.pm)
        self.assertIn('PMHOME.EXE', cam.pm)
        self.assertEqual(cam.snapshots, 2)
        with self.assertRaises(cxpm.PmError):
            cxpm.remove(cam, b'helper', ['PMHOME.EXE'], os.path.join(self.out, 'snap3'), log=self.log.append)


if __name__ == '__main__':
    unittest.main()
