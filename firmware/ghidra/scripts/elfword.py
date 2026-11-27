"""Reads 32-bit words from an ELF at Ghidra addresses (file vaddr + 0x10000)."""
import struct, sys
def load(path):
    d = open(path, 'rb').read()
    phoff, = struct.unpack_from('<I', d, 0x1c); phnum, = struct.unpack_from('<H', d, 0x2c)
    segs = []
    for i in range(phnum):
        t, off, va, _, fs, _, _, _ = struct.unpack_from('<8I', d, phoff + 32 * i)
        if t == 1: segs.append((va, off, fs))
    def word(gaddr):
        va = gaddr - 0x10000
        for v, o, s in segs:
            if v <= va < v + s: return struct.unpack_from('<I', d, o + va - v)[0]
        raise KeyError(hex(gaddr))
    return word
if __name__ == '__main__':
    w = load(sys.argv[1])
    for a in sys.argv[2:]:
        print('%s = %#x' % (a, w(int(a, 16))))
