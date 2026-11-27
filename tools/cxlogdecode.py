#!/usr/bin/env python3
"""Decodes /setting/cxreq.log (written by preload/cxg.so) with the libObj.so
1.07 dictionaries in firmware/names/ (MSGID.tsv, PARAMID.tsv, VALUEID.tsv).

    cxlogdecode.py backups/<dir>/cxreq.log            everything, decoded
    cxlogdecode.py cxreq.log --only OA,OS,EX,RD,RW    some line kinds
    cxlogdecode.py cxreq.log --grep IRIS              lines that mention IRIS
    cxlogdecode.py cxreq.log --summary                exposure timeline in plain words

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


GAIN_DB = {0x1e5: 'auto', 0x1e6: '-6 dB', 0x1e7: '-3 dB'}
GAIN_DB.update({0x1e8 + i: '%d dB' % (3 * i) for i in range(11)})
EXP_MODES = {0xf3: 'iris priority', 0xf4: 'shutter priority', 0xf5: 'manual', 0xf7: 'auto'}
DRIVE = {0x1ca: 'iris auto', 0x1cb: 'iris manual', 0x1cc: 'shutter auto', 0x1cd: 'shutter manual',
         0x2d6: 'AE lock on', 0x2d8: 'AE lock off'}


def summarize(line):
    """One plain-words line for the events that matter for exposure, or None."""
    m = re.match(r'^([0-9a-f]+)\.([0-9a-f]+) (\S+)(.*)$', line)
    if not m:
        return None
    sec, frac, kind, rest = m.groups()
    t = '%7.2f ' % (int(sec, 16) + int(frac, 16) / 1024.0)
    if kind == 'RD':
        return t + 'screen' + rest.split('"', 2)[-1]
    if kind == 'OV':
        v = re.search(r'"(view/[^"]+)"', rest)
        return t + 'screen opens ' + v.group(1) if v else None
    if kind == 'NF':
        f = re.search(r'fno=([0-9a-f]+)', rest)
        sh = re.search(r'shut=([0-9a-f]+)/([0-9a-f]+)', rest)
        if f:
            return t + 'camera reports F%.1f' % (int(f.group(1), 16) / 100.0)
        if sh:
            return t + 'camera reports shutter %d/%d' % (int(sh.group(1), 16), int(sh.group(2), 16))
    if kind == 'NI':
        g = re.search(r'p9706:7=([0-9a-f]+)', rest)
        if g:
            return t + 'camera reports gain ' + GAIN_DB.get(int(g.group(1), 16), '0x' + g.group(1))
        return None
    if kind == 'OA':
        e = re.search(r'm=7558 .*p95c8:[0-9a-f]+=([0-9a-f]+)', rest)
        if e:
            return t + 'interface asks exposure mode: ' + EXP_MODES.get(int(e.group(1), 16), e.group(1))
        return None
    if kind == 'IN':
        ret = re.search(r'ret=([0-9a-f]+)', rest)
        failed = '' if ret and ret.group(1) == '0' else '   <-- REJECTED ret=%s' % (ret.group(1) if ret else '?')
        d = re.search(r'm=(75b4|75b6|7610) .*p(96d3|96d6|9795):7=([0-9a-f]+)', rest)
        if d:
            return t + 'cxg sets ' + DRIVE.get(int(d.group(3), 16), d.group(3)) + failed
        a = re.search(r'm=75ba .*p96dc:3=([0-9a-f]+) p96dd:8=([0-9a-f]+)', rest)
        if a:
            cap = 'off' if a.group(2) == '1d1' else '%d dB' % int(a.group(1), 16)
            return t + 'cxg sets gain cap ' + cap + failed
        return t + 'cxg sends' + rest + failed
    if kind in ('FX', 'GL', 'TX', 'TI', 'MN', 'UL', 'AU', 'SY', 'KP', 'ID', 'MP', 'MR', 'KE', 'LB', 'PN'):
        return t + kind + rest
    return None


def main():
    p = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    p.add_argument('log')
    p.add_argument('--only', help='comma separated line kinds (HDR, OV, CV, RD, RW, EX, GI, OA, OS)')
    p.add_argument('--grep', help='regex the decoded line must match')
    p.add_argument('--summary', action='store_true', help='exposure timeline in plain words')
    args = p.parse_args()
    msgs, params, values = load('MSGID.tsv'), load('PARAMID.tsv'), load('VALUEID.tsv')
    only = set(args.only.split(',')) if args.only else None
    with open(args.log, encoding='latin1') as f:
        for line in f:
            line = line.rstrip('\n')
            if args.summary:
                out = summarize(line)
                if out and (not args.grep or re.search(args.grep, out)):
                    print(out)
                continue
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
