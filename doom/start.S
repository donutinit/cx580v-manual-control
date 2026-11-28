/*
 * Cabecera de CXDOOM.BIN, entradas y setjmp/longjmp.
 *
 * Las entradas corren el juego en una pila propia: la del hilo que llama no se
 * sabe cuánto mide. La de música corre en otro hilo y tiene la suya. No se llama a nada de la cámara desde
 * aquí dentro salvo la función de bitácora del anfitrión.
 */
    .arch armv6
    .arm
    .section .header, "a"
    .global cxd_header
cxd_header:
    .word 0x4d445843            @ "CXDM"
    .word 5
    .word cxd_header
    .word __file_end - cxd_header
    .word __mem_end - cxd_header
    .word 0                     @ suma: la pone doom/build.sh
    .word cxd_entry_init
    .word cxd_entry_tick
    .word cxd_entry_key
    .word 0                     @ entradas de la tabla de reubicación: la pone doom/build.sh
    .word cxd_entry_music
    .space 20

    .text
    .macro ENTRY name, target, stack=cxd_stack_top
    .global \name
    .type \name, %function
\name:
    push {r4, r5, lr}
    mov r4, sp
    ldr r5, =\stack
    mov sp, r5
    bl \target
    mov sp, r4
    pop {r4, r5, lr}
    bx lr
    .size \name, .-\name
    .endm

    ENTRY cxd_entry_init, cxd_init_c
    ENTRY cxd_entry_tick, cxd_tick_c
    ENTRY cxd_entry_key, cxd_key_c
    ENTRY cxd_entry_music, cxd_music_c, cxd_stack2_top   @ otro hilo, otra pila
    .ltorg

    .global cxd_setjmp
    .type cxd_setjmp, %function
cxd_setjmp:
    stmia r0, {r4-r11, sp, lr}
    mov r0, #0
    bx lr

    .global cxd_longjmp
    .type cxd_longjmp, %function
cxd_longjmp:
    ldmia r0, {r4-r11, sp, lr}
    movs r0, r1
    moveq r0, #1
    bx lr

    .bss
    .align 3
    .space 0x40000              @ 256 KiB de pila
cxd_stack_top:
    .space 0x10000              @ 64 KiB para el hilo de música
cxd_stack2_top:
