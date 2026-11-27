"""Installs and removes the cxguard preload on the camera's settings FAT.

Runs inside PMCA's updater shell (see cxdump.py). Background, from the
firmware 1.07 disassembly (docs/research/firmware-analysis.md):

* The boot loader (boot1) boots the updater in nflasha1 whenever
  /setting/updater/mode exists. The updater deletes the flag at start when
  /setting/updater/comp exists, so one updater boot clears it.
* The stock init of the main system reads /setting/env.txt, but only when
  /setting/mode/dmode exists, and passes its VAR=value lines to bootin.elf
  and everything it starts. It parses st_blocks bytes of the file: 32 bytes
  for one 16 KiB cluster.
* Sony's loader aborts the process when an LD_PRELOAD file is missing, so the
  library lives next to env.txt and is written before the trigger.

Writes happen in an order that leaves the camera stock if interrupted:
library, env.txt, then dmode (the trigger). Removal goes the other way.
"""

import hashlib
import io
import re
import struct

SETTING = '/setting'
SO_PATH = '/setting/cxg.so'
INH_PATH = '/setting/cxinh.bin'
REQ_PATH = '/setting/cxreq.log'
MODE_PATH = '/setting/cxmode.txt'
#: Test modes the library understands (preload/cxview.c): Exposure opens this view.
MODES = ('iris', 'shutter', 'agc', 'aeshift', 'wbshift')
#: Allowed in `redirect <source> <target>` lines (same lists as preload/cxview.c).
SOURCES = ('view/EXPOSURE', 'view/SPOT_METER', 'view/SPOT_FOCUS', 'view/SPOT_METER_FOCUS',
           'view/TELEMACRO', 'view/FOCUS', 'view/WHITE_BALANCE')
TARGETS = ('view/IRIS', 'view/SHUTTER_SPEED', 'view/AGC_LIMIT', 'view/AE_SHIFT', 'view/WB_SHIFT',
           'view/EXPOSURE')
MAX_REDIRECTS = 8
MAX_CONFIG = 1023
MAX_LABELS = 24
MAX_IMAGES = 24
MAX_TEXTS = 4
MAX_MENU_RULES = 32
MAX_HIDEBOXES = 4
MAX_HIDEIDS = 12
#: Menu tables the library knows (offset of the root in viewUnified7.so 1.07) and their highest item id.
MENUS = {'c347c': 254, 'b7220': 77, 'bb980': 205, 'c0380': 27}


def check_config(text):
    """Rejects anything the library would ignore, so a typo never goes unnoticed."""
    if len(text.encode('ascii')) > MAX_CONFIG:
        raise GuardError('config is longer than %d bytes' % MAX_CONFIG)
    redirects = 0
    labels = 0
    images = 0
    texts = 0
    menu_rules = 0
    hideboxes = hideids = 0
    for n, line in enumerate(text.splitlines(), 1):
        w = line.split()
        if not w:
            continue
        if len(w) == 1 and w[0] in MODES:
            redirects += 1
        elif len(w) == 3 and w[0] == 'redirect':
            if w[1] not in SOURCES or w[2] not in TARGETS:
                raise GuardError('line %d: redirect %s -> %s is not allowed' % (n, w[1], w[2]))
            redirects += 1
        elif len(w) == 2 and w[0] in ('aemode', 'aeauto') and w[1].isdigit() and 1 <= int(w[1]) <= 6:
            pass
        elif len(w) == 2 and w[0] == 'gainlock' and w[1] in tuple('0123456789'):
            pass
        elif len(w) == 3 and w[0] == 'label' and all(_is_text_id(x) for x in w[1:]):
            labels += 1
        elif len(w) == 3 and w[0] in ('show', 'hide') and _is_menu_item(w[1], w[2]):
            menu_rules += 1
        elif len(w) == 3 and w[0] == 'image' and all(_is_text_id(x) for x in w[1:]):
            images += 1
        elif len(w) == 3 and w[0] == 'text' and _is_text_id(w[1]) and _is_free_text(w[2]):
            texts += 1
        elif len(w) == 2 and w[0] == 'uxlog' and w[1] in ('0', '1'):
            pass
        elif len(w) == 2 and w[0] == 'panel' and w[1] in ('0', '1', '2'):
            pass
        elif len(w) == 6 and w[0] == 'panelz' and w[1] in tuple('01234') and all(x.isdigit() for x in w[2:]) \
                and int(w[4]) >= 16 and int(w[5]) >= 16 and int(w[2]) + int(w[4]) <= 640 and int(w[3]) + int(w[5]) <= 480:
            pass
        elif len(w) == 5 and w[0] == 'hidebox' and all(x.isdigit() for x in w[1:]) \
                and int(w[3]) >= 8 and int(w[4]) >= 8 and int(w[1]) + int(w[3]) <= 720 and int(w[2]) + int(w[4]) <= 560:
            hideboxes += 1
        elif len(w) == 2 and w[0] == 'hidew' and 1 <= len(w[1]) <= 8 and all(c in '0123456789abcdefABCDEF' for c in w[1]):
            hideids += 1
        elif len(w) == 2 and w[0] == 'paneldump' and w[1] in ('0', '1'):
            pass
        elif len(w) == 3 and w[0] == 'panelfont' and w[1] in tuple('01234') \
                and 1 <= len(w[2]) <= 8 and all(c in '0123456789abcdefABCDEF' for c in w[2]):
            pass
        elif len(w) == 2 and w[0] == 'paneldraw' and w[1] in ('0', '1'):
            pass
        elif len(w) == 2 and w[0] == 'textlog' and w[1] in ('0', '1'):
            pass
        elif len(w) == 2 and w[0] in ('objmanual', 'holdmanual', 'gainshift', 'isoshift', 'expogain', 'unlockiso', 'autounlock') and w[1] in ('0', '1'):
            pass
        else:
            raise GuardError('line %d not understood: %r' % (n, line))
    if redirects > MAX_REDIRECTS:
        raise GuardError('more than %d redirects' % MAX_REDIRECTS)
    if labels > MAX_LABELS:
        raise GuardError('more than %d labels' % MAX_LABELS)
    if images > MAX_IMAGES:
        raise GuardError('more than %d images' % MAX_IMAGES)
    if texts > MAX_TEXTS:
        raise GuardError('more than %d texts' % MAX_TEXTS)
    if menu_rules > MAX_MENU_RULES:
        raise GuardError('more than %d show/hide rules' % MAX_MENU_RULES)
    if hideboxes > MAX_HIDEBOXES:
        raise GuardError('more than %d hidebox lines' % MAX_HIDEBOXES)
    if hideids > MAX_HIDEIDS:
        raise GuardError('more than %d hidew lines' % MAX_HIDEIDS)


def _is_menu_item(root, item):
    """A known menu table of viewUnified7.so 1.07 and an id inside its state array."""
    return root in MENUS and item.isdigit() and 1 <= int(item) <= MENUS[root]


def _is_free_text(word):
    """One word of printable ASCII, 1-23 characters ('_' stands for a space)."""
    return 1 <= len(word) <= 23 and all(33 <= ord(c) <= 126 for c in word)


def _is_text_id(word):
    """Low 16 bits of a 0x0204xxxx text id or 0x0220xxxx image id: 1-4 hex digits, not zero."""
    return 1 <= len(word) <= 4 and all(c in '0123456789abcdefABCDEF' for c in word) and int(word, 16) != 0
#: Files the library writes; guard-fetch copies them, guard-remove deletes them.
DUMP_PATHS = (INH_PATH, REQ_PATH)
ENV_PATH = '/setting/env.txt'
DMODE_PATH = '/setting/mode/dmode'
FLAG_PATH = '/setting/updater/mode'
COMP_PATH = '/setting/updater/comp'

ENV_CONTENT = ('LD_PRELOAD=%s\n' % SO_PATH).encode('ascii')
DMODE_CONTENT = b''

#: init only parses the first st_blocks bytes of env.txt (32 for one cluster).
ENV_PARSE_LIMIT = 32
#: Below this much free space init runs `dosfsck -Y` on /setting at every boot.
FSCK_THRESHOLD = 0x32001
CLUSTER = 16384
#: Free space kept above FSCK_THRESHOLD after our files and the library's dumps
#: (inhibit table, 16 KiB at most; UI log, 48 KiB) are written.
MARGIN = 64 * 1024
MAX_SO_SIZE = 64 * 1024

EM_ARM = 40
ET_DYN = 3


class GuardError(Exception):
    pass


def md5(data):
    return hashlib.md5(data).hexdigest()


def check_library(data):
    """Rejects anything that is not a small 32-bit little-endian ARM shared object."""
    if len(data) < 52 or data[:4] != b'\x7fELF':
        raise GuardError('library is not an ELF file')
    if data[4] != 1 or data[5] != 1:
        raise GuardError('library is not 32-bit little endian')
    e_type, e_machine = struct.unpack_from('<HH', data, 16)
    if e_type != ET_DYN or e_machine != EM_ARM:
        raise GuardError('library is not an ARM shared object (type %d, machine %d)' % (e_type, e_machine))
    if len(data) > MAX_SO_SIZE:
        raise GuardError('library is %d bytes, more than %d' % (len(data), MAX_SO_SIZE))


class CameraFS(object):
    """File operations on the camera, through a cxsession.Session."""

    def __init__(self, session):
        self.session = session

    def exists(self, path):
        status, _ = self.session.sh('test -e %s' % path)
        return status == 0

    def md5(self, path):
        return self.session.remote_md5(path)

    def push(self, path, data):
        self.session.backend.writeFile(path, io.BytesIO(data))

    def remove(self, path):
        self.session.sh('rm -f %s' % path)

    def free_bytes(self, mount):
        status, out = self.session.sh('df %s' % mount)
        for line in out.splitlines():
            f = line.split()
            if len(f) >= 6 and f[-1] == mount and f[-3].isdigit():
                return int(f[-3]) * 1024
        raise GuardError('could not read free space of %s from df:\n%s' % (mount, out))

    def sync(self):
        self.session.sh('sync')


def status(fs):
    """What is on the camera now. Returns a dict."""
    s = {}
    for name, path in (('library', SO_PATH), ('env', ENV_PATH), ('dmode', DMODE_PATH),
                       ('flag', FLAG_PATH), ('comp', COMP_PATH), ('inhdump', INH_PATH),
                       ('reqlog', REQ_PATH), ('mode', MODE_PATH)):
        s[name] = fs.md5(path) if fs.exists(path) else None
    s['free'] = fs.free_bytes(SETTING)
    return s


def _push_verified(fs, path, data):
    fs.push(path, data)
    got = fs.md5(path)
    if got != md5(data):
        raise GuardError('%s: md5 on camera %s, expected %s' % (path, got, md5(data)))


def clusters(nbytes):
    return (nbytes + CLUSTER - 1) // CLUSTER


def clusters_to_add(fs, library):
    """Clusters the install and the library's dumps can still take on /setting.

    Files that already exist are not counted again. An existing library is
    assumed to take at least one cluster, which the new one reuses.
    """
    n = clusters(len(library)) - (1 if fs.exists(SO_PATH) else 0)
    n += 0 if fs.exists(ENV_PATH) else 1
    n += 0 if fs.exists(INH_PATH) else clusters(16 * 1024)   # two 8 KiB snapshots
    n += 0 if fs.exists(REQ_PATH) else clusters(48 * 1024)
    return max(n, 0)


def check_install(fs, library):
    """Raises GuardError unless installing is safe right now."""
    check_library(library)
    if len(ENV_CONTENT) > ENV_PARSE_LIMIT:
        raise GuardError('env.txt line is %d bytes; init only reads %d' % (len(ENV_CONTENT), ENV_PARSE_LIMIT))
    if not fs.exists(SETTING + '/Backup.bin'):
        raise GuardError('/setting is not mounted (no Backup.bin)')
    if not fs.exists(COMP_PATH):
        raise GuardError('%s is missing: the updater would not clear the boot flag by itself' % COMP_PATH)
    if fs.exists(FLAG_PATH):
        raise GuardError('%s already exists: the camera would boot the updater again' % FLAG_PATH)
    for path, want in ((ENV_PATH, ENV_CONTENT), (DMODE_PATH, DMODE_CONTENT)):
        if fs.exists(path) and fs.md5(path) != md5(want):
            raise GuardError('%s exists with content that is not ours; not overwriting it' % path)
    need = FSCK_THRESHOLD + MARGIN + CLUSTER * clusters_to_add(fs, library)
    free = fs.free_bytes(SETTING)
    if free < need:
        raise GuardError('/setting has %d bytes free, needs %d' % (free, need))


def install(fs, library, mode=None, config=None):
    """Installs the library. mode: one of MODES; config: cxmode.txt text. Neither: no test mode."""
    if mode is not None and mode not in MODES:
        raise GuardError('unknown mode %r (known: %s)' % (mode, ', '.join(MODES)))
    if mode is not None and config is not None:
        raise GuardError('give either a mode or a config, not both')
    if mode is not None:
        config = mode + '\n'
    if config is not None:
        check_config(config)
    check_install(fs, library)
    # Upgrading: switch the trigger off before touching the library, so a cut
    # in the middle of the copy can never leave dmode pointing at half a file.
    if fs.exists(DMODE_PATH):
        fs.remove(DMODE_PATH)
        if fs.exists(DMODE_PATH):
            raise GuardError('could not remove %s before upgrading' % DMODE_PATH)
        fs.sync()
    _push_verified(fs, SO_PATH, library)
    _push_verified(fs, ENV_PATH, ENV_CONTENT)
    if config is None:
        fs.remove(MODE_PATH)
    else:
        _push_verified(fs, MODE_PATH, config.encode('ascii'))
    _push_verified(fs, DMODE_PATH, DMODE_CONTENT)
    fs.sync()
    return status(fs)


def remove(fs):
    for path in (DMODE_PATH, ENV_PATH, SO_PATH, MODE_PATH) + DUMP_PATHS:
        fs.remove(path)
        if fs.exists(path):
            raise GuardError('could not remove %s' % path)
        fs.sync()
    return status(fs)


def arm_flag(fs):
    """Writes the boot flag from the shell: the next boot must be the updater.

    This is the test of the recovery path without any code of ours: if the
    camera boots the updater after this, the boot loader honours the flag.
    """
    if not fs.exists(COMP_PATH):
        raise GuardError('%s is missing: the updater would keep booting' % COMP_PATH)
    _push_verified(fs, FLAG_PATH, b'')
    fs.sync()
    return status(fs)


def fetch_dumps(fs, session, outdir, delete):
    """Copies every dump the library wrote off the camera. Returns manifest entries."""
    import os
    entries = []
    for path in DUMP_PATHS:
        if not fs.exists(path):
            continue
        entries.append(session.pull_verified(path, os.path.join(outdir, os.path.basename(path))))
        if delete:
            fs.remove(path)
            if fs.exists(path):
                raise GuardError('could not remove %s' % path)
            fs.sync()
    if not entries:
        raise GuardError('no dumps on the camera yet (%s)' % ', '.join(DUMP_PATHS))
    return entries
