"""Offline tests for cxguard against an in-memory camera file system."""

import os
import struct
import sys
import unittest

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import cxguard


class FakeFS(object):
    def __init__(self, files=None, free=448 * 1024, fail_push=None, corrupt=None):
        self.files = dict(files or {})
        self.free = free
        self.fail_push = fail_push   # path whose push raises (USB cut)
        self.corrupt = corrupt       # path whose push lands corrupted
        self.log = []

    def exists(self, path):
        return path in self.files or path == cxguard.SETTING

    def md5(self, path):
        return cxguard.md5(self.files[path]) if path in self.files else None

    def push(self, path, data):
        self.log.append(('push', path))
        if path == self.fail_push:
            raise IOError('USB transfer failed')
        self.files[path] = data + b'x' if path == self.corrupt else data

    def remove(self, path):
        self.log.append(('rm', path))
        self.files.pop(path, None)

    def free_bytes(self, mount):
        return self.free

    def sync(self):
        self.log.append(('sync',))


def library(machine=cxguard.EM_ARM, etype=cxguard.ET_DYN, size=5244):
    head = b'\x7fELF\x01\x01\x01' + b'\0' * 9 + struct.pack('<HH', etype, machine)
    return head + b'\0' * (size - len(head))


def stock():
    return {'/setting/Backup.bin': b'B', cxguard.COMP_PATH: b''}


class InstallTest(unittest.TestCase):
    def test_installs_library_then_env_then_trigger(self):
        fs = FakeFS(stock())
        s = cxguard.install(fs, library())
        pushes = [p for op, *p in fs.log if op == 'push']
        self.assertEqual(pushes, [[cxguard.SO_PATH], [cxguard.ENV_PATH], [cxguard.DMODE_PATH]])
        self.assertNotIn(cxguard.MODE_PATH, fs.files)
        self.assertEqual(fs.files[cxguard.ENV_PATH], b'LD_PRELOAD=/setting/cxg.so\n')
        self.assertEqual(fs.files[cxguard.DMODE_PATH], b'')
        self.assertIsNone(s['flag'])

    def test_mode_is_written_before_trigger(self):
        fs = FakeFS(stock())
        cxguard.install(fs, library(), mode='iris')
        self.assertEqual(fs.files[cxguard.MODE_PATH], b'iris\n')
        pushes = [p[0] for op, *p in fs.log if op == 'push']
        self.assertLess(pushes.index(cxguard.MODE_PATH), pushes.index(cxguard.DMODE_PATH))

    def test_reinstall_without_mode_clears_it(self):
        fs = FakeFS(stock())
        cxguard.install(fs, library(), mode='iris')
        cxguard.install(fs, library())
        self.assertNotIn(cxguard.MODE_PATH, fs.files)

    def test_v5_config_is_valid_and_written(self):
        here = os.path.dirname(os.path.abspath(__file__))
        with open(os.path.join(here, '..', 'preload', 'cxmode-v5.txt')) as f:
            text = f.read()
        fs = FakeFS(stock())
        cxguard.install(fs, library(), config=text)
        self.assertEqual(fs.files[cxguard.MODE_PATH], text.encode('ascii'))

    def test_shipped_configs_are_valid(self):
        here = os.path.dirname(os.path.abspath(__file__))
        for name in ('cxmode-v9.txt', 'cxmode-v12.txt', 'cxmode-v13.txt', 'cxmode-v14.txt', 'cxmode-v15.txt', 'cxmode-v15b.txt', 'cxmode-v16.txt', 'cxmode-v18.txt', 'cxmode-v18b.txt', 'cxmode-v18c.txt', 'cxmode-v18d.txt', 'cxmode-v19u.txt', 'cxmode-v19k.txt',
                     'cxmode-v18r.txt', 'cxmode-v18u.txt'):
            with open(os.path.join(here, '..', 'preload', name)) as f:
                text = f.read()
            fs = FakeFS(stock())
            cxguard.install(fs, library(), config=text)
            self.assertEqual(fs.files[cxguard.MODE_PATH], text.encode('ascii'), msg=name)

    def test_rejects_bad_config(self):
        for bad in ('redirect view/EXPOSURE view/ROOT\n', 'redirect view/ROOT view/IRIS\n',
                    'aemode 7\n', 'aemode x\n', 'isoshift 2\n', 'gainlock 12\n', 'label 2614\n', 'label 0 28b6\n', 'label 2614 xyz\n',
                    'label 12345 28b6\n', 'label 1 2\n' * 25, 'image 24\n', 'image 1 2\n' * 17, 'text 2690\n', 'text 2690 ' + 'x' * 24 + '\n',
                    'text 1 a\n' * 5, 'uxlog 2\n', 'show c347c 0\n', 'show c347c 255\n', 'hide 12345 12\n',
                    'show c347c\n', 'hide c347c 1\n' * 33, 'rm -rf /\n', 'x' * 2000):
            with self.assertRaises(cxguard.GuardError, msg=bad):
                cxguard.install(FakeFS(stock()), library(), config=bad)

    def test_rejects_unknown_mode(self):
        with self.assertRaises(cxguard.GuardError):
            cxguard.install(FakeFS(stock()), library(), mode='rm -rf /')

    def test_env_line_fits_what_init_parses(self):
        self.assertLessEqual(len(cxguard.ENV_CONTENT), cxguard.ENV_PARSE_LIMIT)

    def test_interrupted_install_never_leaves_trigger_without_library(self):
        for cut in (cxguard.SO_PATH, cxguard.ENV_PATH, cxguard.DMODE_PATH):
            fs = FakeFS(stock(), fail_push=cut)
            with self.assertRaises(IOError):
                cxguard.install(fs, library())
            if cxguard.DMODE_PATH in fs.files:
                self.assertIn(cxguard.SO_PATH, fs.files)
                self.assertIn(cxguard.ENV_PATH, fs.files)

    def test_corrupted_library_stops_before_trigger(self):
        fs = FakeFS(stock(), corrupt=cxguard.SO_PATH)
        with self.assertRaises(cxguard.GuardError):
            cxguard.install(fs, library())
        self.assertNotIn(cxguard.ENV_PATH, fs.files)
        self.assertNotIn(cxguard.DMODE_PATH, fs.files)

    def test_refuses_without_comp(self):
        files = stock()
        del files[cxguard.COMP_PATH]
        with self.assertRaisesRegex(cxguard.GuardError, 'comp'):
            cxguard.install(FakeFS(files), library())

    def test_refuses_when_flag_already_set(self):
        files = dict(stock(), **{cxguard.FLAG_PATH: b''})
        with self.assertRaisesRegex(cxguard.GuardError, 'boot the updater'):
            cxguard.install(FakeFS(files), library())

    def test_refuses_foreign_env(self):
        files = dict(stock(), **{cxguard.ENV_PATH: b'FOO=bar\n'})
        with self.assertRaisesRegex(cxguard.GuardError, 'not ours'):
            cxguard.install(FakeFS(files), library())

    def test_reinstall_over_own_files_is_allowed(self):
        fs = FakeFS(stock())
        cxguard.install(fs, library())
        cxguard.install(fs, library())

    def test_refuses_low_free_space(self):
        with self.assertRaisesRegex(cxguard.GuardError, 'free'):
            cxguard.install(FakeFS(stock(), free=300 * 1024), library())

    def test_space_counts_only_what_is_added(self):
        # The camera on 2026-11-26: v2 installed, an inhibit dump present, 400 KiB free.
        files = dict(stock(), **{cxguard.SO_PATH: b'v2', cxguard.ENV_PATH: cxguard.ENV_CONTENT,
                                 cxguard.DMODE_PATH: b'', cxguard.INH_PATH: b'I'})
        fs = FakeFS(files, free=400 * 1024)
        self.assertEqual(cxguard.clusters_to_add(fs, library(size=16036)), 3)
        cxguard.install(fs, library(size=16036))

    def test_fresh_install_counts_dumps(self):
        self.assertEqual(cxguard.clusters_to_add(FakeFS(stock()), library(size=16036)), 1 + 1 + 1 + 3)

    def test_v5b_upgrade_fits_after_moving_dumps(self):
        # The camera on 2026-11-26 18:18: v5 installed, dumps just moved off, 384 KiB free.
        files = dict(stock(), **{cxguard.SO_PATH: b'v5', cxguard.ENV_PATH: cxguard.ENV_CONTENT,
                                 cxguard.DMODE_PATH: b'', cxguard.MODE_PATH: b'iris\n'})
        cxguard.install(FakeFS(files, free=384 * 1024), library(size=16588), config='aeauto 6\n')

    def test_rejects_non_arm_library(self):
        for bad in (b'not an elf', library(machine=62), library(etype=2), library(size=100 * 1024)):
            with self.assertRaises(cxguard.GuardError):
                cxguard.install(FakeFS(stock()), bad)

    def test_refuses_when_setting_not_mounted(self):
        with self.assertRaisesRegex(cxguard.GuardError, 'mounted'):
            cxguard.install(FakeFS({cxguard.COMP_PATH: b''}), library())


    def test_upgrade_switches_trigger_off_before_library(self):
        fs = FakeFS(stock())
        cxguard.install(fs, library())
        fs.log = []
        cxguard.install(fs, library(size=6000))
        ops = [tuple(x) for x in fs.log if x[0] in ('push', 'rm')]
        self.assertEqual(ops[:2], [('rm', cxguard.DMODE_PATH), ('push', cxguard.SO_PATH)])
        self.assertEqual(len(fs.files[cxguard.SO_PATH]), 6000)
        self.assertIn(cxguard.DMODE_PATH, fs.files)

    def test_interrupted_upgrade_leaves_trigger_off(self):
        fs = FakeFS(stock())
        cxguard.install(fs, library())
        fs.fail_push = cxguard.SO_PATH
        with self.assertRaises(IOError):
            cxguard.install(fs, library(size=6000))
        self.assertNotIn(cxguard.DMODE_PATH, fs.files)


class RemoveTest(unittest.TestCase):
    def test_removes_trigger_first(self):
        fs = FakeFS(stock())
        cxguard.install(fs, library())
        fs.log = []
        s = cxguard.remove(fs)
        removed = [p for op, *p in fs.log if op == 'rm']
        self.assertEqual(removed, [[cxguard.DMODE_PATH], [cxguard.ENV_PATH], [cxguard.SO_PATH], [cxguard.MODE_PATH],
                                   [cxguard.INH_PATH], [cxguard.REQ_PATH]])
        self.assertEqual((s['library'], s['env'], s['dmode']), (None, None, None))
        self.assertEqual(set(fs.files), set(stock()))

    def test_remove_on_stock_camera_is_harmless(self):
        fs = FakeFS(stock())
        cxguard.remove(fs)
        self.assertEqual(fs.files, stock())


class FetchTest(unittest.TestCase):
    class Session(object):
        def __init__(self, fs):
            self.fs = fs
        def pull_verified(self, remote, local):
            return {'remote': remote, 'file': local, 'size': len(self.fs.files[remote])}

    def test_fetches_and_deletes_every_dump(self):
        fs = FakeFS(dict(stock(), **{cxguard.INH_PATH: b'I', cxguard.REQ_PATH: b'R'}))
        entries = cxguard.fetch_dumps(fs, self.Session(fs), '/tmp/x', delete=True)
        self.assertEqual([e['remote'] for e in entries], [cxguard.INH_PATH, cxguard.REQ_PATH])
        self.assertNotIn(cxguard.INH_PATH, fs.files)
        self.assertNotIn(cxguard.REQ_PATH, fs.files)

    def test_keeps_dumps_without_delete(self):
        fs = FakeFS(dict(stock(), **{cxguard.REQ_PATH: b'R'}))
        cxguard.fetch_dumps(fs, self.Session(fs), '/tmp/x', delete=False)
        self.assertIn(cxguard.REQ_PATH, fs.files)

    def test_no_dumps_is_an_error(self):
        fs = FakeFS(stock())
        with self.assertRaises(cxguard.GuardError):
            cxguard.fetch_dumps(fs, self.Session(fs), '/tmp/x', delete=True)


class ArmFlagTest(unittest.TestCase):
    def test_writes_empty_flag(self):
        fs = FakeFS(stock())
        s = cxguard.arm_flag(fs)
        self.assertEqual(fs.files[cxguard.FLAG_PATH], b'')
        self.assertIsNotNone(s['flag'])

    def test_refuses_without_comp(self):
        with self.assertRaises(cxguard.GuardError):
            cxguard.arm_flag(FakeFS({'/setting/Backup.bin': b'B'}))


if __name__ == '__main__':
    unittest.main()
