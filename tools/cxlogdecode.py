#!/usr/bin/env python3
"""Decodes /setting/cxreq.log (written by preload/cxg.so) with the libObj.so
1.07 dictionaries in firmware/names/ (MSGID.tsv, PARAMID.tsv, VALUEID.tsv).

    cxlogdecode.py backups/<dir>/cxreq.log            everything, decoded
    cxlogdecode.py cxreq.log --only OA,OS,EX,RD,RW    some line kinds
    cxlogdecode.py cxreq.log --grep IRIS              lines that mention IRIS

Timestamps are seconds since im.elf started (hex in the file, decimal here).
Values are decoded as VALUEID names only when the parameter is a mode/enum
(name ends in _MODE, _SET or _ACT); other values stay as numbers.
"""

import argparse
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
NAMES = os.path.join(ROOT, 'firmware', 'names')


def load(name):
    table = {}
    with open(os.path.join(NAMES, name)) as f:
        for line in f:
            k, v = line.rstrip('\n').split('\t')
            table[int(k, 16)] = v
    return table


def strip(prefix, name):
    return name[len(prefix):] if name.startswith(prefix) else name


def decode_value(pname, value, valueids):
    enum = re.search(r'_(MODE|SET|ACT|SW|STATUS)$', pname) or pname.endswith('_MODE')
    if enum and value in valueids:
        return strip('VALUEID_', valueids[value])
    if value >= 0x80000000:
        return str(value - (1 << 32))
    return hex(value)


def decode_line(line, msgs, params, values):
    m = re.match(r'^([0-9a-f]+)\.([0-9a-f]+) (\S+)(.*)$', line)
    if not m:
        return line
    sec, frac, kind, rest = m.groups()
    t = '%7.2f' % (int(sec, 16) + int(frac, 16) / 1024.0)
    if kind in ('OA', 'OS'):
        def param(pm):
            pid, typ, val = int(pm.group(1), 16), int(pm.group(2), 16), int(pm.group(3), 16)
            pname = params.get(pid, 'p%#x' % pid)
            return ' %s=%s' % (strip('PARAMID_', pname), decode_value(pname, val, values))
        rest = re.sub(r' m=([0-9a-f]+)', lambda mm: ' ' + strip('MSGID_', msgs.get(int(mm.group(1), 16), 'm=' + mm.group(1))), rest)
        rest = re.sub(r' p([0-9a-f]+):([0-9a-f]+)=([0-9a-f]+)', param, rest)
        rest = re.sub(r' c=3100', '', rest)
    return '%s %-3s%s' % (t, kind, rest)


def main():
    p = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    p.add_argument('log')
    p.add_argument('--only', help='comma separated line kinds (HDR, OV, CV, RD, RW, EX, GI, OA, OS)')
    p.add_argument('--grep', help='regex the decoded line must match')
    args = p.parse_args()
    msgs, params, values = load('MSGID.tsv'), load('PARAMID.tsv'), load('VALUEID.tsv')
    only = set(args.only.split(',')) if args.only else None
    with open(args.log, encoding='latin1') as f:
        for line in f:
            line = line.rstrip('\n')
            parts = line.split(' ', 2)
            if only and (len(parts) < 2 or parts[1] not in only):
                continue
            out = decode_line(line, msgs, params, values)
            if args.grep and not re.search(args.grep, out):
                continue
            print(out)
    return 0


if __name__ == '__main__':
    sys.exit(main())
