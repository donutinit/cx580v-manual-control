"""Pasa los cuadros crudos de doom/test/host (328x200, 16 bits R G B A de alto a
bajo) a una sola imagen PNG, uno al lado del otro, para verlos."""
import struct, sys, zlib

PITCH, W, H = 328, 320, 200


def rows(path):
    d = open(path, 'rb').read()
    px = struct.unpack('<%dH' % (PITCH * H), d)
    out = []
    for y in range(H):
        r = bytearray()
        for x in range(W):
            v = px[y * PITCH + x]
            r += bytes((((v >> 12) & 15) * 17, ((v >> 8) & 15) * 17, ((v >> 4) & 15) * 17))
        out.append(r)
    return out


def png(path, lines, w):
    def chunk(t, d):
        return struct.pack('>I', len(d)) + t + d + struct.pack('>I', zlib.crc32(t + d))
    raw = b''.join(b'\0' + bytes(l) for l in lines)
    open(path, 'wb').write(b'\x89PNG\r\n\x1a\n' + chunk(b'IHDR', struct.pack('>IIBBBBB', w, len(lines), 8, 2, 0, 0, 0)) +
                           chunk(b'IDAT', zlib.compress(raw, 9)) + chunk(b'IEND', b''))


if __name__ == '__main__':
    out, frames = sys.argv[1], [rows(p) for p in sys.argv[2:]]
    cols = 3
    lines = []
    for i in range(0, len(frames), cols):
        group = frames[i:i + cols]
        for y in range(H):
            line = bytearray()
            for f in group:
                line += f[y]
            line += bytes(3 * W * (cols - len(group)))
            lines.append(line)
    png(out, lines, W * cols)
