/* CXDOOM.BIN: imagen plana enlazada en CXD_BASE (preload/cxd.h); se carga en cualquier dirección. */
ENTRY(cxd_header)
SECTIONS
{
    . = 0x70000000;
    .text : { KEEP(*(.header)) *(.text .text.*) *(.rodata .rodata.*) }
    . = ALIGN(8);
    .data : { *(.data .data.*) }
    . = ALIGN(8);
    __file_end = .;
    .bss : { *(.bss .bss.*) *(COMMON) }
    . = ALIGN(4096);
    __mem_end = .;
    /DISCARD/ : { *(.ARM.exidx*) *(.ARM.extab*) *(.comment) *(.note*) *(.ARM.attributes) }
}
