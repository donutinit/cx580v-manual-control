#!/usr/bin/env python3
"""Boots the camera into Sony's firmware-updater mode and runs code there.

    cxdump.py recon   --model 0x21010001          record what system this is
    cxdump.py dump    --model 0x21010001          verified backup of the flash
    cxdump.py verify  backups/<dir>               re-check a backup on disk
    cxdump.py shell   --model 0x21010001          PMCA's interactive shell

How it works: the camera reboots from its separate updater partition and loads
the "updater body" library from the image we hand it. Ours (PMCA's updater
shell) opens a command channel over USB instead of flashing anything. The
image carries an empty firmware section, so the main firmware is not touched.
On exit the camera reboots normally.

recon and dump only read. They never write to flash or to the settings store.
Use cxprobe.py guess-model first to learn --model/--region.
"""

import argparse
import io
import json
import os
import sys
import time
import traceback

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import cxlib
import cxprobe
import cxsession


def run_in_updater(model, region, work):
    """Boots updater mode with our payload and calls work(session) inside it."""
    from pmca.commands.usb import updaterShellCommand
    from pmca.platform.backend.usb import UsbPlatformBackend

    failure = []
    ran = []

    def complete(device):
        backend = UsbPlatformBackend(device)
        backend.start()
        ran.append(True)
        try:
            work(cxsession.Session(backend))
        except BaseException as e:
            # Never leave the camera parked in the updater: record the error,
            # then let the normal exit sequence run so it reboots cleanly.
            traceback.print_exc()
            failure.append(e)
        finally:
            backend.stop()

    payload = cxlib.build_payload(model, region)

    def start():
        updaterShellCommand(fdatFile=io.BytesIO(payload), driverName='libusb', complete=complete)

    start()
    if not ran and not failure and wait_for_updater():
        # The HDR-CX580V takes about 35 s to reboot into updater mode, longer
        # than the 30 s PMCA waits. Once it is there, PMCA picks it up directly.
        print('Camera is now in updater mode, continuing')
        start()
    if failure:
        raise failure[0]
    if not ran:
        # PMCA returns normally when no camera is found or when the camera is
        # slower to reboot than its 30 s wait. Nothing ran, so say so.
        raise NotInUpdater('The updater shell never started. If the camera is still '
                           'rebooting into updater mode, run the same command again.')


def wait_for_updater(timeout=120):
    """Waits for the camera to show up in updater mode. Returns whether it did."""
    import usb.core
    from pmca.usb.sony import SONY_ID_VENDOR, SONY_ID_PRODUCT_UPDATER
    print('Waiting up to %d s for the camera to finish rebooting...' % timeout)
    deadline = time.time() + timeout
    while time.time() < deadline:
        if any(d.idProduct in SONY_ID_PRODUCT_UPDATER for d in usb.core.find(find_all=True, idVendor=SONY_ID_VENDOR)):
            time.sleep(3)  # let the kernel finish binding its own driver first
            return True
        time.sleep(1)
    return False


class NotInUpdater(Exception):
    pass


def default_outdir(kind):
    return os.path.join(cxlib.ROOT, 'backups', time.strftime('%Y%m%d-%H%M%S') + '-' + kind)


def cmd_recon(args):
    out = args.out or default_outdir('recon')

    def work(session):
        cxsession.recon(session, out)
        for cmd in args.extra or []:
            status, output = session.sh(cmd)
            with open(os.path.join(out, 'recon.txt'), 'a') as f:
                f.write('### %s (exit %d)\n%s\n' % (cmd, status, output))

    run_in_updater(args.model, args.region, work)
    print('Recon written to %s' % os.path.join(out, 'recon.txt'))


def cmd_dump(args):
    out = args.out or default_outdir('dump')
    result = {}

    def work(session):
        result['manifest'] = cxsession.dump(session, out, include_large=args.include_large, only=args.only)

    run_in_updater(args.model, args.region, work)
    m = result['manifest']
    print('Backup written to %s' % out)
    for p in m['partitions']:
        print('  %-14s %10d bytes  sha256 %s  [%s]' % (p['file'], p['size'], p['sha256'][:16], p['verified']))
    for e in m['extra']:
        print('  %-14s %10d bytes  sha256 %s' % (e['file'], e['size'], e['sha256'][:16]))
    for s in m['skipped']:
        print('  skipped %s (%d MiB): %s' % (s['name'], s['size'] >> 20, s['reason']))


def cmd_verify(args):
    try:
        bad = cxsession.verify_manifest(args.dir)
    except (OSError, UnicodeError, json.JSONDecodeError, cxsession.VerifyError) as exc:
        print('Cannot verify backup: %s' % exc, file=sys.stderr)
        return 1
    if bad:
        print('CORRUPT or missing: %s' % ', '.join(ascii(name) for name in bad))
        return 1
    print('OK: every file matches manifest.json')


def cmd_shell(args):
    from pmca.commands.usb import updaterShellCommand
    updaterShellCommand(fdatFile=io.BytesIO(cxlib.build_payload(args.model, args.region)), driverName='libusb')


def print_guard_status(s):
    for name in ('library', 'env', 'dmode', 'flag', 'comp', 'inhdump', 'reqlog', 'mode'):
        print('  %-8s %s' % (name, s[name] or '-'))
    print('  free     %d KiB' % (s['free'] // 1024))


def confirm(args, what):
    if args.yes:
        return True
    print('This WRITES to the camera settings partition (/setting): %s' % what)
    return input('Type "si" to continue: ').strip().lower() == 'si'


def snapshot_setting(session, kind):
    """Verified copy of nflasha2 (the /setting FAT) before changing it."""
    out = default_outdir(kind)
    os.makedirs(out)
    session.sh('sync')
    entry = session.pull_verified('/dev/nflasha2', os.path.join(out, 'nflasha2.bin'))
    with open(os.path.join(out, 'manifest.json'), 'w') as f:
        json.dump({'created': time.strftime('%Y-%m-%dT%H:%M:%S%z'), 'partitions': [entry], 'extra': []}, f, indent=2)
    print('Snapshot of /setting written to %s' % out)
    return out


def cmd_guard(args):
    import cxguard
    # Libraries we released, so the one on the camera can be sized by its md5.
    release = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), 'preload', 'release')
    if os.path.isdir(release):
        for name in os.listdir(release):
            if name.endswith('.so'):
                with open(os.path.join(release, name), 'rb') as f:
                    data = f.read()
                cxguard.KNOWN_LIBRARIES[cxguard.md5(data)] = len(data)
    library = None
    config = None
    if args.cmd == 'guard-install':
        with open(args.library, 'rb') as f:
            library = f.read()
        cxguard.check_library(library)
        config = None
        if args.config:
            with open(args.config) as f:
                config = f.read()
            cxguard.check_config(config)
    actions = {
        'guard-status': None,
        'guard-flag-test': 'create /setting/updater/mode, so the next boot is the updater',
        'guard-install': 'add %s, %s and %s%s' % (cxguard.SO_PATH, cxguard.ENV_PATH, cxguard.DMODE_PATH,
                                                  (' with test mode %s' % args.mode) if getattr(args, 'mode', None) else
                                                  (' with config %s' % args.config) if getattr(args, 'config', None) else ''),
        'guard-remove': 'delete %s, %s, %s and %s' % (cxguard.DMODE_PATH, cxguard.ENV_PATH, cxguard.SO_PATH, cxguard.INH_PATH),
        'guard-fetch': ('delete %s after copying them' % ', '.join(cxguard.DUMP_PATHS)) if getattr(args, 'delete', False) else None,
    }
    if getattr(args, 'doom', False):
        doom_files()                   # fail before rebooting the camera if one is missing
        actions[args.cmd] += '; and CXDOOM.BIN and DOOM1.WAD to the PlayMemories partition (/dev/nflasha12)'
    if actions[args.cmd] and not confirm(args, actions[args.cmd]):
        print('Cancelled, nothing was sent to the camera.')
        return 1

    def work(session):
        fs = cxguard.CameraFS(session)
        print('Before:')
        print_guard_status(cxguard.status(fs))
        if args.cmd == 'guard-status':
            return
        if args.cmd == 'guard-fetch':
            out = default_outdir('dumps')
            os.makedirs(out)
            entries = cxguard.fetch_dumps(fs, session, out, args.delete)
            with open(os.path.join(out, 'manifest.json'), 'w') as f:
                json.dump({'created': time.strftime('%Y-%m-%dT%H:%M:%S%z'), 'partitions': [], 'extra': entries}, f, indent=2)
            print('Dumps written to %s: %s' % (out, ', '.join(e['file'] for e in entries)))
            print('After:')
            print_guard_status(cxguard.status(fs))
            return
        if args.cmd == 'guard-install':
            cxguard.check_install(fs, library)
        snap = snapshot_setting(session, args.cmd)
        if args.cmd == 'guard-install' and any(fs.exists(p) for p in cxguard.DUMP_PATHS):
            # The library writes each dump once per install: clear old ones so
            # the new version starts recording. They are kept with the snapshot.
            entries = cxguard.fetch_dumps(fs, session, snap, delete=True)
            print('Old dumps moved to %s: %s' % (snap, ', '.join(e['file'] for e in entries)))
        s = {'guard-flag-test': lambda: cxguard.arm_flag(fs),
             'guard-install': lambda: cxguard.install(fs, library, args.mode, config),
             'guard-remove': lambda: cxguard.remove(fs)}[args.cmd]()
        print('After:')
        print_guard_status(s)
        # In the same updater session, so the owner does not have to enter it twice.
        if getattr(args, 'doom', False):
            pm_install(session)
        elif getattr(args, 'pm_check', False):
            pm_check(session)

    run_in_updater(args.model, args.region, work)


DOOM_FILES = (('CXDOOM.BIN', os.path.join('doom', 'build', 'CXDOOM.BIN')),
              ('DOOM1.WAD', os.path.join('vendor', 'doom', 'wad', 'doom1.wad')))


def doom_files():
    """The DOOM program and the shareware WAD, as {NAME: bytes}."""
    root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    files = {}
    for name, rel in DOOM_FILES:
        with open(os.path.join(root, rel), 'rb') as f:
            files[name] = f.read()
    return files


def pm_check(session):
    import cxpm
    info = cxpm.check(session, cxpm.load_helper())
    print('PlayMemories partition (%s), read-only look:\n%s\nfree %d KiB' % (cxpm.DEVICE, info['listing'], info['free'] // 1024))


def pm_install(session):
    import cxpm
    cxpm.install(session, cxpm.load_helper(), doom_files(), default_outdir('doom-install'))


def cmd_pm(args):
    import cxpm
    what = {'pm-check': None,
            'doom-install': 'add %s to the PlayMemories partition (%s)' % (' and '.join(n for n, _ in DOOM_FILES), cxpm.DEVICE),
            'doom-remove': 'delete %s from the PlayMemories partition (%s)' % (' and '.join(n for n, _ in DOOM_FILES), cxpm.DEVICE)}[args.cmd]
    if args.cmd == 'doom-install':
        doom_files()                   # fail before rebooting the camera if one is missing
    if what and not args.yes:
        print('This WRITES to the camera flash: %s' % what)
        if input('Type "si" to continue: ').strip().lower() != 'si':
            print('Cancelled, nothing was sent to the camera.')
            return 1

    def work(session):
        if args.cmd == 'pm-check':
            pm_check(session)
        elif args.cmd == 'doom-install':
            pm_install(session)
        else:
            cxpm.remove(session, cxpm.load_helper(), [n for n, _ in DOOM_FILES], default_outdir('doom-remove'))

    run_in_updater(args.model, args.region, work)


def main():
    p = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = p.add_subparsers(dest='cmd', required=True)

    def camera_args(s):
        s.add_argument('--model', type=lambda v: int(v, 0), required=True, help='updater model id (cxprobe.py guess-model)')
        s.add_argument('--region', type=lambda v: int(v, 0), default=0)
        s.add_argument('--unmount', action='store_true', help='unmount the camera storage first')

    s = sub.add_parser('recon')
    camera_args(s)
    s.add_argument('--out')
    s.add_argument('--extra', action='append', metavar='CMD', help='extra shell command to record (repeatable)')
    s = sub.add_parser('dump')
    camera_args(s)
    s.add_argument('--out')
    s.add_argument('--only', action='append', metavar='NAME', help='dump only this partition (repeatable)')
    s.add_argument('--include-large', action='store_true', help='also dump partitions over 2 GiB (user storage)')
    s = sub.add_parser('verify')
    s.add_argument('dir')
    s = sub.add_parser('shell')
    camera_args(s)
    for name in ('guard-status', 'guard-flag-test', 'guard-install', 'guard-remove', 'guard-fetch'):
        s = sub.add_parser(name)
        camera_args(s)
        s.add_argument('--yes', action='store_true', help='do not ask for confirmation')
        if name == 'guard-install':
            s.add_argument('--library', default=os.path.join(cxlib.ROOT, 'preload', 'cxg.so'))
            s.add_argument('--mode', choices=('iris', 'shutter', 'agc', 'aeshift', 'wbshift'),
                           help='test mode: the Exposure screen opens this view instead')
            s.add_argument('--config', help='cxmode.txt to install (see preload/cxview.c), e.g. preload/cxmode-v5.txt')
            s.add_argument('--pm-check', action='store_true',
                           help='afterwards, mount the PlayMemories partition read-only and list it (writes nothing)')
            s.add_argument('--doom', action='store_true',
                           help='afterwards, copy doom/build/CXDOOM.BIN and the shareware WAD to the PlayMemories partition')
        if name == 'guard-fetch':
            s.add_argument('--delete', action='store_true', help='delete the dump from the camera after copying it')

    for name in ('pm-check', 'doom-install', 'doom-remove'):
        s = sub.add_parser(name)
        camera_args(s)
        s.add_argument('--yes', action='store_true', help='do not ask for confirmation')

    args = p.parse_args()
    if args.cmd != 'verify' and not cxprobe.ensure_unmounted(args.unmount):
        return 1
    commands = {'recon': cmd_recon, 'dump': cmd_dump, 'verify': cmd_verify, 'shell': cmd_shell}
    if args.cmd.startswith('guard-'):
        commands[args.cmd] = cmd_guard
    if args.cmd in ('pm-check', 'doom-install', 'doom-remove'):
        commands[args.cmd] = cmd_pm
    try:
        return commands[args.cmd](args) or 0
    except NotInUpdater as e:
        print('Error: %s' % e, file=sys.stderr)
        return 2


if __name__ == '__main__':
    sys.exit(main())
