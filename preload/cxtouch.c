/*
 * Tacto: envolturas de WidgetSystem::postMouseMove/Press/Release, que InputService
 * llama por la PLT en el hilo de la interfaz (docs/research/ux-panel-propio.md;
 * probadas en la cámara el 2026-11-27 con la variante de bitácora).
 *
 * Son trampolines en ensamblador: guardan los registros, llaman a una función
 * que decide y devuelve a dónde saltar, y saltan con los argumentos y la pila
 * tal como llegaron. Si el toque es del panel propio (cxpanel.c) saltan a una
 * función vacía y la interfaz no lo ve; si no, a la original.
 *
 * Un dedo llega así: Move(x, y) y Press al apoyar, Move al arrastrar, Release y
 * Move(fuera de pantalla) al soltar. El Move de antes del Press pasa siempre;
 * si el Press cae en el panel, se tragan ese Press, los Move del arrastre y el
 * Release.
 */
#include "cx.h"

#ifndef CX_TOUCH_LIB
#define CX_TOUCH_LIB "/libObj.so"
#endif
#define SYM_MMOVE  "_ZN2ux6wgtsys12WidgetSystem13postMouseMoveEhssNS_4core8MsecTimeE"
#define SYM_MPRESS "_ZN2ux6wgtsys12WidgetSystem14postMousePressEhhNS_4core8MsecTimeE"
#define SYM_MREL   "_ZN2ux6wgtsys12WidgetSystem16postMouseReleaseEhhNS_4core8MsecTimeE"

static u32 r_move, r_press, r_rel;
static volatile int resolving;
static int last_x, last_y, captured;

/* Si la original no aparece, el toque se pierde pero la aplicación no se cae. */
static u32 nothing(void) { return 0; }

static void resolve(void)
{
    static int tries;
    u32 base;
    if ((r_move && r_press && r_rel) || tries >= 3) return;
    if (__sync_lock_test_and_set(&resolving, 1)) return;   /* cx_lib_base usa un búfer estático */
    tries++;
    base = cx_lib_base(CX_TOUCH_LIB);
    r_move = cx_dlsym(base, SYM_MMOVE);
    r_press = cx_dlsym(base, SYM_MPRESS);
    r_rel = cx_dlsym(base, SYM_MREL);
    __sync_lock_release(&resolving);
}

__attribute__((used, visibility("hidden")))
u32 cxtouch_note_move(u32 ws, u32 mouse, u32 x, u32 y)
{
    (void)ws; (void)mouse;
    resolve();
    last_x = (short)x;
    last_y = (short)y;
#ifdef CX_UX
    if (cx_uxlog && cx_log_enabled()) cxux_flush();   /* los botones que anotaron otros hilos */
#endif
    cxpanel_hook(4);
    if (captured) {
        if (cxdoom_active()) cxdoom_touch(2, last_x, last_y);
        return (u32)nothing;
    }
    return r_move ? r_move : (u32)nothing;
}

__attribute__((used, visibility("hidden")))
u32 cxtouch_note_press(u32 ws, u32 mouse, u32 button, u32 t)
{
    (void)ws; (void)t;
    resolve();
#ifdef CX_UX
    cxux_touch("MP", mouse, button, last_x, last_y);
#else
    (void)mouse; (void)button;
#endif
    captured = 0;                    /* un Press con otro pendiente: se perdió un Release */
    if (cxdoom_active()) {           /* con el juego en pantalla, todo el tacto es suyo */
        cxdoom_touch(1, last_x, last_y);
        captured = 1;
        return (u32)nothing;
    }
    if (cxpanel_press(last_x, last_y)) {
        captured = 1;
        return (u32)nothing;
    }
    return r_press ? r_press : (u32)nothing;
}

__attribute__((used, visibility("hidden")))
u32 cxtouch_note_release(u32 ws, u32 mouse, u32 button, u32 t)
{
    (void)ws; (void)t;
    resolve();
#ifdef CX_UX
    cxux_touch("MR", mouse, button, last_x, last_y);
#else
    (void)mouse; (void)button;
#endif
    if (captured) {
        captured = 0;
        cxdoom_touch(0, 0, 0);
        cxpanel_hook(4);
        return (u32)nothing;
    }
    return r_rel ? r_rel : (u32)nothing;
}

#define TRAMPOLINE(sym, note) __asm__( \
    ".text\n.arm\n.align 2\n.global " sym "\n.type " sym ", %function\n" sym ":\n" \
    "    push {r0-r3, r12, lr}\n" \
    "    bl " note "\n" \
    "    mov r12, r0\n" \
    "    pop {r0-r3}\n" \
    "    add sp, sp, #4\n" \
    "    pop {lr}\n" \
    "    bx r12\n" \
    ".size " sym ", .-" sym "\n")

TRAMPOLINE(SYM_MMOVE, "cxtouch_note_move");
TRAMPOLINE(SYM_MPRESS, "cxtouch_note_press");
TRAMPOLINE(SYM_MREL, "cxtouch_note_release");

void cxtouch_start(void)
{
    resolve();
}
