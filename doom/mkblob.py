"""Termina CXDOOM.BIN: le agrega la tabla de reubicación y llena la cabecera.

El programa se enlaza sin código independiente de la posición (es más rápido)
en CXD_BASE, pero la cámara decide dónde cabe. «ld -q» deja en el ELF los
registros de reubicación; aquí se juntan los R_ARM_ABS32, que son las únicas
palabras que dependen de la dirección (datos con punteros y las constantes que
el código carga), y se escriben como una lista de desplazamientos detrás de la
imagen. El anfitrión le suma a cada una la diferencia entre la dirección real y
la de enlace.

    mkblob.py cxdoom.elf CXDOOM.BIN
"""
import struct
import sys

R_ARM_NONE, R_ARM_PC24, R_ARM_ABS32, R_ARM_REL32 = 0, 1, 2, 3
R_ARM_CALL, R_ARM_JUMP24, R_ARM_V4BX, R_ARM_PREL31 = 28, 29, 40, 42
RELATIVE = (R_ARM_NONE, R_ARM_PC24, R_ARM_REL32, R_ARM_CALL, R_ARM_JUMP24, R_ARM_V4BX, R_ARM_PREL31)
SHT_SYMTAB, SHT_REL, SHT_RELA, SHF_ALLOC = 2, 9, 4, 2
SHN_UNDEF, SHN_ABS = 0, 0xfff1
HEADER = 64


def relocations(elf):
    """Addresses of every word that holds an absolute address."""
    assert elf[:6] == b'\x7fELF\x01\x01', 'not a 32-bit little-endian ELF'
    shoff, = struct.unpack_from('<I', elf, 0x20)
    shentsize, shnum = struct.unpack_from('<HH', elf, 0x2e)
    sections = [struct.unpack_from('<10I', elf, shoff + i * shentsize) for i in range(shnum)]
    out = []
    for name, kind, flags, addr, offset, size, link, info, align, entsize in sections:
        assert kind != SHT_RELA, 'RELA sections are not expected on ARM'
        if kind != SHT_REL or not sections[info][2] & SHF_ALLOC:
            continue
        assert sections[link][1] == SHT_SYMTAB
        symoff = sections[link][4]
        for i in range(size // 8):
            where, rinfo = struct.unpack_from('<II', elf, offset + i * 8)
            rtype, sym = rinfo & 0xff, rinfo >> 8
            if rtype in RELATIVE:
                continue
            if rtype != R_ARM_ABS32:
                raise SystemExit('relocation type %d at %#x is not handled' % (rtype, where))
            shndx, = struct.unpack_from('<H', elf, symoff + sym * 16 + 14)
            if shndx == SHN_ABS:
                continue                      # a constant, not an address
            if shndx == SHN_UNDEF:
                raise SystemExit('absolute relocation against an undefined symbol at %#x' % where)
            out.append(where)
    return sorted(set(out))


def main(elf_path, bin_path):
    elf = open(elf_path, 'rb').read()
    image = bytearray(open(bin_path, 'rb').read())
    magic, version, base, file_size, mem_size = struct.unpack_from('<5I', image, 0)
    assert magic == 0x4d445843 and version == 5, (hex(magic), version)
    # objcopy no escribe el relleno de alineación que hay entre el fin de .data y __file_end.
    assert file_size - 8 < len(image) <= file_size and file_size % 4 == 0, (file_size, len(image))
    image += bytes(file_size - len(image))
    table = [a - base for a in relocations(elf)]
    assert all(0 <= off <= file_size - 4 and off % 4 == 0 for off in table), 'relocation outside the image or unaligned'
    # The host reads the table into the zeroed area behind the image, so it must fit there.
    assert len(table) * 4 <= mem_size - file_size, 'the relocation table does not fit behind the image'
    struct.pack_into('<I', image, 36, len(table))
    image += struct.pack('<%dI' % len(table), *table)
    total = sum(struct.unpack_from('<%dI' % ((len(image) - HEADER) // 4), image, HEADER)) & 0xffffffff
    struct.pack_into('<I', image, 20, total)
    open(bin_path, 'wb').write(image)
    print('CXDOOM.BIN %d bytes (imagen %d, %d reubicaciones), en memoria %d, suma %08x' % (
        len(image), file_size, len(table), mem_size, total))


if __name__ == '__main__':
    main(sys.argv[1], sys.argv[2])
