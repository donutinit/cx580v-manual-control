#!/usr/bin/env python3
"""Read-only reconnaissance of a Sony camera connected over USB.

    cxprobe.py usb          list Sony USB devices and their descriptors
    cxprobe.py info         model name, product code, serial, firmware version
    cxprobe.py guess-model  find the updater model/region id of the camera

Nothing here changes the camera: `info` only issues query commands, and
`guess-model` uses the updater's "check guard" step, the same check Sony's own
updater runs before it asks the user whether to proceed. The camera stays in
normal mode throughout; no mode switch is requested and nothing is written.
"""

import argparse
import io
import os
import plistlib
import subprocess
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import cxlib

SONY_VENDOR = '054c'


def sony_block_devices():
    """Yields (disk, [(partition, mountpoint)]) for block devices on a Sony USB device."""
    mounts = {}
    with open('/proc/self/mounts') as f:
        for line in f:
            dev, mnt = line.split()[:2]
            mounts.setdefault(os.path.realpath(dev), []).append(mnt)
    for disk in sorted(os.listdir('/sys/block')):
        path = os.path.realpath(os.path.join('/sys/block', disk, 'device'))
        vendor = None
        while path != '/':
            idv = os.path.join(path, 'idVendor')
            if os.path.isfile(idv):
                with open(idv) as f:
                    vendor = f.read().strip()
                break
            path = os.path.dirname(path)
        if vendor != SONY_VENDOR:
            continue
        nodes = [disk] + sorted(p for p in os.listdir(os.path.join('/sys/block', disk)) if p.startswith(disk))
        yield disk, [(n, m) for n in nodes for m in mounts.get('/dev/' + n, [])]


def ensure_unmounted(unmount):
    """libusb takes the device away from usb-storage, so nothing may be mounted."""
    busy = [(n, m) for _, parts in sony_block_devices() for n, m in parts]
    if not busy:
        return True
    for node, mnt in busy:
        if unmount:
            print('Unmounting /dev/%s (%s)' % (node, mnt))
            subprocess.run(['udisksctl', 'unmount', '-b', '/dev/' + node], check=True)
        else:
            print('Error: /dev/%s is mounted at %s. Unmount it first or pass --unmount.' % (node, mnt))
    return unmount


def open_device():
    from pmca.commands.usb import importDriver, getDevice
    driver = importDriver('libusb')
    return driver, getDevice


def cmd_usb(args):
    import usb.core
    found = False
    for dev in usb.core.find(find_all=True, idVendor=int(SONY_VENDOR, 16)):
        found = True
        print('%04x:%04x  bus %d addr %d  speed %s' % (dev.idVendor, dev.idProduct, dev.bus, dev.address, dev.speed))
        for cfg in dev:
            for intf in cfg:
                print('  interface %d: class 0x%02x subclass 0x%02x protocol 0x%02x, %d endpoints' % (
                    intf.bInterfaceNumber, intf.bInterfaceClass, intf.bInterfaceSubClass,
                    intf.bInterfaceProtocol, intf.bNumEndpoints))
    for disk, parts in sony_block_devices():
        print('  block device /dev/%s%s' % (disk, ''.join(' [%s on %s]' % p for p in parts)))
    if not found:
        print('No Sony USB device found.')
        return 1


def cmd_info(args):
    if not ensure_unmounted(args.unmount):
        return 1
    from pmca.usb.sony import SonyExtCmdCamera, SonyExtCmdDevice, SonyUpdaterCamera
    driver, getDevice = open_device()
    with driver:
        device = getDevice(driver)
        if not device:
            return 1
        if not isinstance(device, SonyExtCmdDevice):
            print('Camera is not in mass storage / MTP mode: %s' % type(device).__name__)
            return 1
        info = SonyExtCmdCamera(device).getCameraInfo()
        updater = SonyUpdaterCamera(device)
        updater.init()
        version, _ = updater.getFirmwareVersion()
        print('Model name:       %s' % info.modelName)
        print('Product code:     %s' % info.modelCode)
        print('Serial number:    %s' % info.serial)
        print('Firmware version: %s' % version)
        try:
            plist = plistlib.loads(info.plist)
            print('Device plist:')
            for k, v in sorted(plist.items()):
                print('  %-28s %r' % (k, v))
        except Exception as e:
            print('Device plist (raw, %s): %r' % (e, info.plist[:400]))
        if args.plist:
            with open(args.plist, 'wb') as f:
                f.write(info.plist)
            print('Raw plist written to %s' % args.plist)


def check_guard(updater, model, region):
    """Runs the updater guard check. Returns None on success, else the reason."""
    data = cxlib.build_payload(model, region)
    updater.init()
    try:
        updater.checkGuard(io.BytesIO(data), len(data))
        return None
    except Exception as e:
        return str(e).replace('Firmware update error: ', '')


def cmd_guess_model(args):
    if not ensure_unmounted(args.unmount):
        return 1
    from pmca.usb.sony import SonyExtCmdDevice, SonyUpdaterCamera
    driver, getDevice = open_device()
    with driver:
        device = getDevice(driver)
        if not device or not isinstance(device, SonyExtCmdDevice):
            print('Camera not found in mass storage / MTP mode.')
            return 1
        updater = SonyUpdaterCamera(device)

        shipped = cxlib.model_ids()
        names = {}
        for name, (model, _) in shipped.items():
            names.setdefault(model, []).append(name)

        # Siblings first, then every other id PMCA knows, then a sweep of the
        # 2012 camcorder range (0x2101xxxx).
        siblings = ['HDR-PJ590V', 'HDR-PJ760V', 'HDR-TD20']
        candidates = [shipped[n][0] for n in siblings]
        candidates += [m for m, _ in shipped.values()]
        if args.sweep:
            candidates += range(0x21010000, 0x21010000 + args.sweep)
        seen = set()
        for model in candidates:
            if model in seen:
                continue
            seen.add(model)
            reason = check_guard(updater, model, 0)
            label = '/'.join(names.get(model, ['?']))
            if reason == 'Invalid model':
                if args.verbose:
                    print('0x%08x %-28s invalid model' % (model, label))
                continue
            if reason and reason != 'Invalid region':
                # Battery, AC adapter or media conditions are checked before
                # the model, so report them instead of mis-attributing them.
                print('0x%08x %-28s guard refused: %s' % (model, label, reason))
                print('Fix the reported condition and run this again.')
                return 1
            print('MODEL MATCH: 0x%08x (%s)' % (model, label))
            region = 0
            while reason == 'Invalid region' and region < 16:
                region += 1
                reason = check_guard(updater, model, region)
            if reason:
                print('Region not found (last answer: %s)' % reason)
                return 1
            print('REGION MATCH: %d' % region)
            print('Guard check passed for model 0x%08x region %d.' % (model, region))
            return 0
        print('No model id matched (%d tried). Re-run with --sweep N to scan 0x21010000+.' % len(seen))
        return 1


def main():
    p = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = p.add_subparsers(dest='cmd', required=True)
    sub.add_parser('usb')
    s = sub.add_parser('info')
    s.add_argument('--unmount', action='store_true', help='unmount the camera storage first')
    s.add_argument('--plist', help='write the raw device info plist to this file')
    s = sub.add_parser('guess-model')
    s.add_argument('--unmount', action='store_true', help='unmount the camera storage first')
    s.add_argument('--sweep', type=int, default=0, metavar='N', help='also scan N ids from 0x21010000')
    s.add_argument('-v', '--verbose', action='store_true')
    args = p.parse_args()
    return {'usb': cmd_usb, 'info': cmd_info, 'guess-model': cmd_guess_model}[args.cmd](args) or 0


if __name__ == '__main__':
    sys.exit(main())
