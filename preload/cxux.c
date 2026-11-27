/*
 * Bitácora de tacto y botones: el primer paso hacia un panel propio. Sólo entra
 * en la variante cxg-ux.so (-DCX_UX); cxg.so no la lleva.
 *
 * Detalle del análisis en docs/research/ux-panel-propio.md. En resumen:
 *  - El tacto llega al framework ux por WidgetSystem::postMouseMove/Press/
 *    Release; esas envolturas están en cxtouch.c, que avisa aquí (cxux_touch).
 *  - Los botones físicos llegan como eventos de la aplicación por
 *    EventManager::push(Event*), desde hilos de Infra; el número de evento está
 *    en ev+4 (0x1502xxxx teclas, 0x150bxxxx palancas, 0x1503xxxx diales).
 *
 * Aquí no se traga ni se cambia nada. Las envolturas son trampolines en
 * ensamblador: guardan los registros, llaman a una función que anota y devuelve
 * la dirección de la original, y saltan a ella con los argumentos y la pila tal
 * como llegaron. Así no dependen de cómo se pasen los argumentos extra.
 *
 * cxmode.txt:  uxlog 1    anota toques, botones y la fuente de las etiquetas
 */
#include "cx.h"

#ifdef CX_UX

#ifndef CX_UX_LIB
#define CX_UX_LIB "/libObj.so"
#endif
#define SYM_EVPUSH "_ZN12EventManager4pushEP5Event"

#define MAX_TOUCH 60
#define MAX_KEYS  80
#define MAX_FONTS 12
#define RING      64          /* potencia de 2 */

#define SYM_UPDRES "_ZN2ux6gfxeng14GraphicsEngine14updateResourceEv"

int cx_uxlog;
static u32 r_push, r_updres;
static volatile int resolving;
static int n_touch, n_keys, n_fonts;
static volatile u32 ring[RING];
static volatile u32 ring_w;
static u32 ring_r;

/* Si la original no aparece, el evento se pierde pero la app no se cae. */
static u32 nothing(void) { return 0; }

static void resolve(void)
{
    static int tries;
    u32 base;
    if ((r_push && r_updres) || tries >= 3) return;
    if (__sync_lock_test_and_set(&resolving, 1)) return;   /* cx_lib_base usa un búfer estático */
    tries++;
    base = cx_lib_base(CX_UX_LIB);
    r_push = cx_dlsym(base, SYM_EVPUSH);
    r_updres = cx_dlsym(base, SYM_UPDRES);
    __sync_lock_release(&resolving);
}

static u32 tid(void) { return (u32)sys3(224 /* gettid */, 0, 0, 0); }

/* Vuelca, desde el hilo de la interfaz, los botones que anotaron otros hilos. */
void cxux_flush(void)
{
    struct line l;
    while (ring_r != ring_w && n_keys < MAX_KEYS) {
        u32 id = ring[ring_r++ & (RING - 1)];
        n_keys++;
        cxl_begin(&l, "KE");
        cxl_lit(&l, " ");
        cxl_hex(&l, id);
        cxl_end(&l);
    }
}

/* Lo llama cxtouch.c en cada apoyar (MP) y soltar (MR). */
void cxux_touch(const char *kind, u32 mouse, u32 button, int x, int y)
{
    struct line l;
    if (!cx_uxlog || !cx_log_enabled() || n_touch >= MAX_TOUCH) return;
    n_touch++;
    cxl_begin(&l, kind);
    cxl_lit(&l, " mouse=");
    cxl_hex(&l, mouse & 0xff);
    cxl_lit(&l, " button=");
    cxl_hex(&l, button & 0xff);
    cxl_lit(&l, " x=");
    cxl_hex(&l, (u32)x);
    cxl_lit(&l, " y=");
    cxl_hex(&l, (u32)y);
    cxl_lit(&l, " tid=");
    cxl_hex(&l, tid());
    cxl_end(&l);
}

/*
 * GraphicsEngine::updateResource(): candidata a reloj del hilo de la interfaz.
 * Sólo se cuenta (cxpanel_hook) para saber cada cuánto y en qué hilo corre.
 */
__attribute__((used, visibility("hidden")))
u32 cxux_note_updres(void)
{
    if (!r_updres) resolve();
    cxpanel_hook(5);
    return r_updres ? r_updres : (u32)nothing;
}

/* Corre en el hilo de quien manda el evento: sólo guarda el número. */
__attribute__((used, visibility("hidden")))
u32 cxux_note_push(u32 self, u32 ev)
{
    (void)self;
    if (cx_uxlog && ev && !(ev & 3)) {
        u32 id = ((const u32 *)ev)[1], top = id >> 16;
        if (top == 0x1502 || top == 0x150b || top == 0x1503)
            ring[__sync_fetch_and_add(&ring_w, 1) & (RING - 1)] = id;
    }
    if (!r_push) resolve();
    return r_push ? r_push : (u32)nothing;
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

TRAMPOLINE(SYM_EVPUSH, "cxux_note_push");
TRAMPOLINE(SYM_UPDRES, "cxux_note_updres");

/*
 * Fuente de las etiquetas de fábrica: seis palabras en GEN_Label + 0x264. El
 * TextSprite que recibe setString(id) está en GEN_Label + 0x144, así que quedan
 * en sprite + 0x120. Hacen falta para crear una etiqueta propia después.
 */
void cxux_note_label(u32 sprite, u32 id)
{
    static u32 seen[MAX_FONTS];
    struct line l;
    const u32 *f = (const u32 *)(sprite + 0x120);
    int i;
    if (!cx_uxlog || !cx_log_enabled() || n_fonts >= MAX_FONTS || !sprite || (sprite & 3)) return;
    for (i = 0; i < n_fonts; i++)
        if (seen[i] == id) return;
    seen[n_fonts++] = id;
    cxl_begin(&l, "LB");
    cxl_lit(&l, " id=");
    cxl_hex(&l, id);
    cxl_lit(&l, " label=");
    cxl_hex(&l, sprite - 0x144);
    cxl_lit(&l, " font");
    for (i = 0; i < 6; i++) {
        cxl_lit(&l, " ");
        cxl_hex(&l, f[i]);
    }
    cxl_lit(&l, " tid=");
    cxl_hex(&l, tid());
    cxl_end(&l);
}

void cxux_start(void)
{
    resolve();
}

#endif /* CX_UX */
