#!/usr/bin/env python3
"""Reads /setting/cxinh.bin, the inhibit-table snapshots written by cxg.so.

Each snapshot: header "CXI1", seconds, feat_max, words, mwf_base, tbl,
status, nonzero_total, nrec (u32 little endian), then nrec records of
(u16 feature, u16 word, u32 value). Bit b of word w set for feature f means
inhibit factor w*32+b currently blocks feature f.

    cxinh.py dump.bin                 summary of every snapshot
    cxinh.py dump.bin --feature 0x174 factors blocking one feature
"""

import argparse
import struct
import sys

HEAD = struct.Struct('<4s8I')
REC = struct.Struct('<HHI')

STATUS = {
    0: 'ok',
    1: 'could not read /proc/self/maps',
    2: 'libMWF.so not mapped',
    3: 'libMWF.so is not the 1.07 build',
    4: 'mp_inhTbl not in mapped memory',
    5: 'table not initialised yet (NULL)',
    6: 'table pointer misaligned',
    7: 'table not in mapped memory',
}


class Snapshot(object):
    def __init__(self, seconds, feat_max, words, mwf_base, tbl, status, nonzero_total, records):
        self.seconds, self.feat_max, self.words = seconds, feat_max, words
        self.mwf_base, self.tbl, self.status = mwf_base, tbl, status
        self.nonzero_total, self.records = nonzero_total, records

    @property
    def truncated(self):
        return len(self.records) < self.nonzero_total

    def factors(self):
        """{feature: sorted list of factor ids blocking it}."""
        out = {}
        for f, w, v in self.records:
            for b in range(32):
                if v >> b & 1:
                    out.setdefault(f, []).append(w * 32 + b)
        return dict((f, sorted(x)) for f, x in out.items())


def parse(data):
    snaps, off = [], 0
    while off < len(data):
        if len(data) - off < HEAD.size:
            raise ValueError('truncated header at offset %d' % off)
        magic, sec, fmax, words, base, tbl, status, total, nrec = HEAD.unpack_from(data, off)
        if magic != b'CXI1':
            raise ValueError('bad magic %r at offset %d' % (magic, off))
        off += HEAD.size
        if len(data) - off < nrec * REC.size:
            raise ValueError('truncated records at offset %d' % off)
        recs = [REC.unpack_from(data, off + i * REC.size) for i in range(nrec)]
        off += nrec * REC.size
        snaps.append(Snapshot(sec, fmax, words, base, tbl, status, total, recs))
    return snaps


def main():
    p = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    p.add_argument('file')
    p.add_argument('--feature', type=lambda v: int(v, 0), action='append')
    args = p.parse_args()
    with open(args.file, 'rb') as f:
        snaps = parse(f.read())
    for i, s in enumerate(snaps):
        fac = s.factors()
        print('snapshot %d at %d s: %s, libMWF at %#x, table at %#x, %d non-zero words%s, %d features blocked' % (
            i + 1, s.seconds, STATUS.get(s.status, 'status %d' % s.status), s.mwf_base, s.tbl,
            s.nonzero_total, ' (truncated)' if s.truncated else '', len(fac)))
        counts = {}
        for f, xs in fac.items():
            for x in xs:
                counts[x] = counts.get(x, 0) + 1
        top = sorted(counts.items(), key=lambda kv: -kv[1])[:15]
        if top:
            print('  most common factors: ' + ', '.join('%#x (%d)' % kv for kv in top))
        for f in args.feature or []:
            print('  feature %#x: %s' % (f, ', '.join('%#x' % x for x in fac.get(f, [])) or 'not blocked'))
    return 0


if __name__ == '__main__':
    sys.exit(main())
