/*
 * DOOM desde el menú de la cámara.
 *
 * El juego es un programa aparte (doom/, doomgeneric con una libc mínima) que
 * vive en /log, la partición de PlayMemories, junto al DOOM1.WAD de la versión
 * shareware. Aquí está lo que lo une a la cámara:
 *
 *  - Cargador: lee /log/CXDOOM.BIN donde el kernel dé sitio, comprueba la
 *    cabecera y la suma, corrige las direcciones con la tabla de reubicación y
 *    llama a su entrada init. Si algo no cuadra, no se corre nada.
 *  - Cuadros: el juego corre en un hilo propio, de prioridad baja, y deja cada
 *    cuadro terminado (ya en 16 bits) en uno de tres búferes. En cada turno del
 *    hilo de la interfaz (cxpanel_hook) sólo se toma el último, se bloquea el
 *    bitmap del cuadro (cxframe.c) sin esperar, se copia y se avisa al sprite.
 *    Antes el juego avanzaba dentro del hilo de la interfaz, y doomgeneric
 *    duerme ahí hasta su siguiente tic (TryRunTics): la interfaz pasaba casi
 *    todo el tiempo dormida y en pantalla no se vio nada (v32 y v33,
 *    2026-11-27). Mientras carga se ven las franjas de la sonda.
 *  - Controles: la palanca de zoom avanza y retrocede, PHOTO dispara, START/STOP
 *    abre puertas, «ver imágenes» es Esc (el menú del juego) y MODE sale. En la
 *    pantalla, el tercio izquierdo y el derecho giran; el centro es Enter
 *    arriba, disparo en medio y cambio de arma abajo. Mientras el juego está en
 *    pantalla esos botones y toques no le llegan a la cámara.
 *  - Menú: abrir la pantalla de un ítem de fábrica (doomview) no la abre; cierra
 *    el menú y arranca el juego sobre la pantalla de grabación, que es donde hay
 *    turnos del hilo de la interfaz. Si el menú no se cierra solo, arranca cuando
 *    lo cierra el usuario.
 *
 * cxmode.txt:  doom 1               disponible desde el menú (necesita panel)
 *              doom 2               además, con un toque en la esquina de arriba a la derecha
 *              doomview view/X      la pantalla que lo lanza
 *              doommb N             MiB para el juego (6)
 *              doomms N             milisegundos mínimos entre cuadros mostrados (0)
 *              doompanel N          ocultar el panel a los N segundos de juego (sin esta
 *                                   línea, nunca). Con el panel oculto el juego no se ve:
 *                                   la v36 lo mostró a los 15 s (2026-11-27).
 *              doompix R G B A      dónde van los 4 bits de cada color y los bits opacos (12 8 4 f)
 *              doomsound 1          efectos de sonido (cxsound.c)
 *              doomhz N             muestras por segundo de los pitidos (24000, la de la cámara)
 *              doomvol N            volumen: escala de 1 a 8 (8)
 *              doommusic 1          música (OPL emulado en el juego, en trozos de pitido);
 *                                   la pantalla de título se queda, con E1M1 en bucle
 *              doomchunk N          milisegundos de música nueva por trozo (1000)
 *              doomlead N           cuánto antes se manda cada trozo (1150: lo que pasa de
 *                                   doomchunk es el solape con fundido entre trozos)
 *              doomrate N           muestras por segundo del emulador OPL (24000)
 */
#include "cx.h"
#include "cxd.h"

#ifndef CX_DOOM_DIR
#define CX_DOOM_DIR "/log"
#endif
#ifndef CX_DOOM_LIB
#define CX_DOOM_LIB "/libObj.so"
#endif
#ifndef CX_DOOM_CLOSE_MS
#define CX_DOOM_CLOSE_MS 500         /* del cierre del submenú al del menú principal */
#endif
#define SYM_EVPUSH "_ZN12EventManager4pushEP5Event"
#define RING 32                      /* potencia de 2 */
#define EV_MODE 0x15020090u

int cx_doom;
u32 cx_doom_ms;                      /* tiempo mínimo entre cuadros mostrados (0: cada turno) */
#define PANEL_ALWAYS 0xffffffffu
u32 cx_doom_panel = PANEL_ALWAYS;    /* segundos que el panel sigue activo debajo del juego */
u32 cx_doom_mb = 6;
/*
 * La tasa de los pitidos: la cámara los reproduce a 24 kHz. A 48 kHz la música
 * salía una octava más grave (v42); con 24 kHz el dueño la oyó en su tono (v43,
 * 2026-11-27).
 */
u32 cx_doom_hz = 24000, cx_doom_vol = 8;
int cx_doom_music;
/* Trozos de 1 s que se encima 150 ms con el anterior (lead - chunk), con fundido. */
u32 cx_doom_mrate = 24000, cx_doom_chunk = 1000, cx_doom_lead = 1150;
u8 cx_doom_pix[4] = { 12, 8, 4, 0xf };
char cx_doom_view[40];

enum { IDLE, ARMED, RUNNING };
static volatile int state;
static volatile int mode_release;    /* MODE sacó del juego: su «soltar» tampoco pasa */
static volatile u32 ring[RING];
static volatile u32 ring_w;
static u32 ring_r;
static u32 r_push;
static volatile int resolving;

static struct cxd_header hd;         /* sólo lo usa el hilo del juego */
static struct cxd_host host;
static u32 mapped, base;
static int step, touch_key, in_sub, pending;
static u32 armed_t, base_t, shown_t, stat_t, run_t, gm_self, n_turn, n_frame, n_busy, n_front, n_stat;
static int panel_off;                /* ya pasó el tiempo con el panel activo */
static char sub_name[40];

/* Hilo del juego. */
static volatile int g_thread, g_loaded, g_ended;
/* Hilo de música: m_hold lo detiene (recarga), m_in dice que está dentro del juego. */
static volatile int m_thread, m_hold = 1, m_in, m_dead;
static volatile u32 g_frames, g_ticks;
static u16 *fb[3];                   /* 320x200 cada uno */
static volatile u32 mbox = 1;        /* búfer del buzón; con el bit 2, uno nuevo sin mostrar */
static u32 game_idx = 0, ui_idx = 2;
#define KRING 64                     /* teclas para el juego: bit 8 apretar, 0..7 la tecla */
static volatile u16 kring[KRING];
static volatile u32 kring_w, kring_r;

static void say(const char *what, u32 a, u32 b)
{
    struct line l;
    if (!cx_log_enabled()) return;
    cxl_begin(&l, "DM");
    cxl_lit(&l, " ");
    cxl_lit(&l, what);
    cxl_lit(&l, " ");
    cxl_hex(&l, a);
    cxl_lit(&l, " ");
    cxl_hex(&l, b);
    cxl_end(&l);
}

/* Lo que imprime el juego. Corre en la pila del juego: sólo llamadas al kernel. */
static void doom_log(const char *s)
{
    struct line l;
    if (!cx_log_enabled()) return;
    cxl_begin(&l, "DM");
    cxl_lit(&l, " | ");
    cxl_str(&l, s, 150);
    cxl_end(&l);
}

/* Milisegundos (de 1.05 ms): cx_now da el resto en 0..953. */
static u32 now(void)
{
    u32 f, s = cx_now(&f);
    return s * 954 + f;
}

static int starts(const char *s, const char *prefix)
{
    for (; *prefix; s++, prefix++)
        if (*s != *prefix) return 0;
    return 1;
}

static int has(const char *s, const char *word)
{
    for (; *s; s++)
        if (starts(s, word)) return 1;
    return 0;
}

int cxdoom_active(void) { return state == RUNNING; }
int cxdoom_hides_panel(void) { return state == RUNNING && panel_off; }

/* --- cargador ---------------------------------------------------------- */

/* Copia palabras; una asignación de estructuras llamaría a memcpy, que aquí no existe. */
static void words(u32 *d, const u32 *s, u32 n)
{
    volatile u32 *v = d;
    while (n--) *v++ = *s++;
}

static int load(void)
{
    long fd = cx_open(CX_DOOM_DIR "/CXDOOM.BIN", O_RDONLY, 0), n;
    u32 got, end, sum = 0, i, delta;
    struct cxd_header h;
    if (fd < 0) { say("no esta " CX_DOOM_DIR "/CXDOOM.BIN", (u32)fd, 0); return 0; }
    /* La tabla de reubicación se lee al sitio que después queda en cero, detrás de la imagen. */
    if (cx_read(fd, &h, sizeof(h)) != (long)sizeof(h) || h.magic != CXD_MAGIC || h.version != CXD_VERSION ||
        h.file_size < sizeof(h) || (h.file_size & 3) || h.file_size > h.mem_size || h.mem_size > (4u << 20) ||
        h.nreloc > (h.mem_size - h.file_size) / 4) {
        say("cabecera mala", h.magic, h.version);
        cx_close(fd);
        return 0;
    }
    if (!mapped) {
        long p = sys6(SYS_mmap2, 0, h.mem_size, 7 /* leer, escribir y ejecutar */, 0x22, -1, 0);
        if ((u32)p > 0xfffff000u) { say("no hay memoria para el programa", (u32)p, h.mem_size); cx_close(fd); return 0; }
        base = (u32)p;
        mapped = h.mem_size;
    } else {
        if (h.mem_size > mapped) { say("la imagen crecio", h.mem_size, mapped); cx_close(fd); return 0; }
        for (i = 0; i < mapped; i += 4) *(volatile u32 *)(base + i) = 0;
    }
    words((u32 *)base, (const u32 *)&h, sizeof(h) / 4);
    end = h.file_size + h.nreloc * 4;
    for (got = sizeof(h); got < end; got += (u32)n) {
        n = cx_read(fd, (void *)(base + got), end - got);
        if (n <= 0) { say("lectura corta", got, end); cx_close(fd); return 0; }
    }
    cx_close(fd);
    for (i = sizeof(h); i < end; i += 4) sum += *(const u32 *)(base + i);
    if (sum != h.sum) { say("suma mala", sum, h.sum); return 0; }
    delta = base - h.base;
    for (i = 0; i < h.nreloc; i++) {
        u32 *slot = (u32 *)(base + h.file_size + i * 4), off = *slot;
        if (off > h.file_size - 4 || (off & 3)) { say("reubicacion mala", i, off); return 0; }
        *(u32 *)(base + off) += delta;
        *slot = 0;
    }
    sys3(0xf0002 /* cacheflush */, base, base + h.file_size, 0);
    words((u32 *)&hd, (const u32 *)base, sizeof(hd) / 4);   /* con las entradas ya en su dirección real */
    say("programa en", base, h.nreloc);
    return 1;
}

static int zoom_dir;

/* Una tecla para el juego: la toma su hilo antes del siguiente turno. */
static void key(int down, int k)
{
    u32 w = kring_w;
    if (!k || w - kring_r >= KRING) return;
    kring[w & (KRING - 1)] = (u16)(k | (down ? 0x100 : 0));
    __sync_synchronize();
    kring_w = w + 1;
    if (k == CXK_ENTER) key(down, CXK_YES);   /* Enter también contesta «y» */
}

static void stats(const char *why)
{
    if (cx_sound) cxsound_stats();
    say(why, n_turn, n_frame);
    say("cuadros del juego, bitmap ocupado", g_frames, n_busy);
    say("traido al frente, estado del cuadro", n_front, cxframe_state());
}

static void stop(const char *why)
{
    static const u8 held[] = { CXK_UP, CXK_DOWN, CXK_LEFT, CXK_RIGHT, CXK_FIRE, CXK_USE };
    unsigned i;
    stats(why);
    for (i = 0; i < sizeof(held); i++) key(0, held[i]);   /* que al volver no siga andando solo */
    state = IDLE;
    touch_key = 0;
    zoom_dir = 0;
    cxframe_clear();                   /* queda transparente: ocultarlo desde aquí no se vería */
    cxpanel_update();
    cx_log_sync();
}

/* Carga y arranca el juego (hilo del juego). */
static int boot(void)
{
    u32 t0 = now();
    int r;
    if (!load()) return 0;
    if (!fb[0]) {
        long p = sys6(SYS_mmap2, 0, 3 * 320 * 200 * 2, 3, 0x22, -1, 0);
        if ((u32)p > 0xfffff000u) { say("no hay memoria para los cuadros", (u32)p, 0); return 0; }
        fb[0] = (u16 *)p;
        fb[1] = fb[0] + 320 * 200;
        fb[2] = fb[1] + 320 * 200;
    }
    host.log = doom_log;
    host.wad = CX_DOOM_DIR "/DOOM1.WAD";
    host.zone_mb = cx_doom_mb;
    host.shift[0] = cx_doom_pix[0];
    host.shift[1] = cx_doom_pix[1];
    host.shift[2] = cx_doom_pix[2];
    host.opaque = cx_doom_pix[3];
    host.sound = cx_sound ? (void (*)(const unsigned char *, unsigned))cxsound_play : 0;
    host.sound_hz = cx_doom_hz;
    host.sound_shift = cx_doom_vol;
    host.music = cx_sound && cx_doom_music ? (void (*)(const unsigned char *, unsigned))cxsound_music : 0;
    host.music_rate = cx_doom_mrate;
    host.music_chunk_ms = cx_doom_chunk;
    host.music_lead_ms = cx_doom_lead;
    host.music_title = 1;
    say("cargado, arranca", hd.file_size, hd.mem_size);
    cx_log_sync();
    r = ((cxd_init_fn)hd.init)(&host);
    say("init, ms", (u32)r, now() - t0);
    cx_log_sync();
    return r == 0;
}

/* Milisegundos de procesador que lleva este hilo (aproximados). */
static u32 cpu_ms(void)
{
    long ts[2] = { 0, 0 };
    sys3(263 /* clock_gettime */, 3 /* CLOCK_THREAD_CPUTIME_ID */, ts, 0);
    return (u32)ts[0] * 1000 + ((u32)ts[1] >> 20);
}

/*
 * El hilo de música: cada 10 ms le pide al juego que genere la música que toca
 * (cxd.c, music_pump). Corre en otro núcleo, así el sintetizador no le quita
 * tiempo al juego (v40: con todo en un hilo, 8 s sin imagen y la música atrasada).
 */
static void music_main(void)
{
    long param = 0;
    sys3(156 /* sched_setscheduler */, 0, 0 /* SCHED_OTHER */, &param);
    sys3(97 /* setpriority */, 0, 0, 5);
    for (;;) {
        if (state != RUNNING || !g_loaded || m_hold || m_dead) { cx_sleep_ms(30); continue; }
        m_in = 1;
        __sync_synchronize();
        if (!m_hold && g_loaded && ((cxd_music_fn)hd.music)() < 0) m_dead = 1;
        m_in = 0;
        cx_sleep_ms(10);
    }
}

static void game_main(void)
{
    long param = 0;
    u32 n = 0, w0 = 0, c0 = 0, gstat = 0;
    say("hilo del juego: politica, nice", (u32)sys3(157 /* sched_getscheduler */, 0, 0, 0),
        (u32)sys3(96 /* getpriority */, 0, 0, 0));
    sys3(156 /* sched_setscheduler */, 0, 0 /* SCHED_OTHER */, &param);
    sys3(97 /* setpriority */, 0 /* PRIO_PROCESS */, 0 /* este hilo */, 10);
    for (;;) {
        int r;
        if (state != RUNNING || g_ended) { cx_sleep_ms(40); continue; }
        if (!g_loaded) {
            m_hold = 1;                /* nada de música mientras se carga la imagen */
            while (m_in) cx_sleep_ms(2);
            if (!boot()) { g_ended = 1; continue; }
            g_loaded = 1;
            m_dead = 0;
            __sync_synchronize();
            m_hold = 0;
            n = 0;
        }
        while (kring_r != kring_w) {
            u32 v;
            __sync_synchronize();
            v = kring[kring_r & (KRING - 1)];
            kring_r++;
            ((cxd_key_fn)hd.key)((v >> 8) & 1, v & 0xff);
        }
        if (!n) { w0 = now(); c0 = cpu_ms(); }
        r = ((cxd_tick_fn)hd.tick)(fb[game_idx], 320);   /* duerme hasta el siguiente tic */
        g_ticks++;
        if (r < 0) { g_loaded = 0; g_ended = 1; continue; }
        if (r > 0) {
            __sync_synchronize();
            game_idx = __sync_lock_test_and_set(&mbox, game_idx | 4) & 3;
            g_frames++;
        }
        if (++n == 256) {
            if (gstat < 8) { gstat++; say("juego: 256 turnos en ms, de procesador", now() - w0, cpu_ms() - c0); }
            n = 0;
        }
    }
}

static void start(void)
{
    state = IDLE;
    if (!g_loaded) {
        /* El juego usa su memoria poco a poco; si no sobra desde ya, mejor no empezar. */
        u32 kb = cxframe_free_kb(), need = (cx_doom_mb + 8) * 1024;
        say("memoria libre y necesaria (kB)", kb, need);
        if (kb < need) { cx_log_sync(); return; }
    }
    if (!cxframe_ready()) {
        /* Debió prepararse al tocar; si no, se intenta igual y queda anotado. */
        say("cuadro creado desde un turno", 0, 0);
        if (!cxframe_prepare()) { say("no hay cuadro", 0, 0); return; }
    }
    if (!g_loaded) cxframe_pattern();  /* las franjas mientras carga: si se ven, el cuadro está */
    if (!g_thread) {
        if (cx_thread(game_main) != 0) { say("no se pudo crear el hilo del juego", 0, 0); cxframe_close(); return; }
        g_thread = 1;
    }
    if (cx_sound && cx_doom_music && !m_thread) {
        if (cx_thread(music_main) != 0) say("no se pudo crear el hilo de musica", 0, 0);
        else m_thread = 1;
    }
    ring_r = ring_w;
    n_turn = n_frame = n_busy = n_front = n_stat = 0;
    stat_t = run_t = now();
    pending = 0;
    panel_off = !cx_doom_panel;
    say("arranca; ya cargado, segundos con panel", (u32)g_loaded, cx_doom_panel);
    state = RUNNING;
    cxpanel_update();
}

/* --- controles ---------------------------------------------------------- */

/* Un botón físico (hilo de la interfaz). Apretar termina en 2 y soltar en 1. */
static void button(u32 id)
{
    u32 k = id & 0xfffffff0u, st = id & 0xf;
    if (k == 0x150b0030u) {            /* palanca de zoom: 2 y 4 hacia T, 3 y 5 hacia W, 1 al centro */
        int d = (st == 2 || st == 4) ? 1 : (st == 3 || st == 5) ? -1 : 0;
        if (d == zoom_dir) return;     /* se repite cada 0.1 s mientras se sostiene */
        if (zoom_dir) key(0, zoom_dir > 0 ? CXK_UP : CXK_DOWN);
        if (d) key(1, d > 0 ? CXK_UP : CXK_DOWN);
        zoom_dir = d;
        return;
    }
    if (st != 1 && st != 2) return;
    if (k == EV_MODE) { if (st == 2) stop("MODE: sale del juego"); return; }
    /* PHOTO a fondo llega después de PHOTO a medias, que ya dispara: no hace nada más.
     * (En la v43 cambiaba la tasa de los pitidos, y disparar fuerte cambiaba el tono.) */
    if (k == 0x15020040u) return;
    key(st == 2, k == 0x15020030u ? CXK_FIRE :      /* PHOTO a medias */
                 k == 0x15020050u ? CXK_USE :       /* START/STOP */
                 k == 0x15020020u ? CXK_ESC : 0);   /* ver imágenes */
}

/* Un toque: kind 1 apoyar, 2 arrastrar, 0 soltar. Coordenadas de 640x480. */
void cxdoom_touch(int kind, int x, int y)
{
    int k = !kind ? 0 : x < 213 ? CXK_LEFT : x >= 427 ? CXK_RIGHT : y < 160 ? CXK_ENTER : y < 320 ? CXK_FIRE : CXK_NEXTW;
    if (k == touch_key) return;
    if (touch_key) key(0, touch_key);
    if (k) key(1, k);
    touch_key = k;
}

static int ours(u32 id)
{
    u32 k = id & 0xfffffff0u;
    return k == 0x150b0030u || k == 0x15020020u || k == 0x15020030u || k == 0x15020040u || k == 0x15020050u || k == EV_MODE;
}

/* Si la original no aparece, el evento se pierde pero la aplicación no se cae. */
static u32 nothing(void) { return 0; }

/*
 * EventManager::push(Event*): los botones físicos, desde hilos de Infra. Aquí
 * sólo se decide si el evento pasa; lo demás se hace en el hilo de la interfaz.
 */
__attribute__((used, visibility("hidden")))
u32 cxdoom_note_push(u32 self, u32 ev)
{
    int eat = 0;
    (void)self;
    if (ev && !(ev & 3)) {
        u32 id = ((const u32 *)ev)[1];
#ifdef CX_UX
        cxux_note_event(id);
#endif
        if (state == RUNNING && ours(id)) {
            ring[__sync_fetch_and_add(&ring_w, 1) & (RING - 1)] = id;
            if (id == EV_MODE + 2) mode_release = 1;
            eat = 1;
        } else if (mode_release && id == EV_MODE + 1) {
            mode_release = 0;
            eat = 1;
        }
    }
    if (!r_push) cxdoom_start();
    return (eat || !r_push) ? (u32)nothing : r_push;
}

/* Al cargar la biblioteca: la original de push, antes de que llegue el primer botón. */
void cxdoom_start(void)
{
    if (r_push || __sync_lock_test_and_set(&resolving, 1)) return;   /* cx_lib_base usa un búfer estático */
    r_push = cx_dlsym(cx_lib_base(CX_DOOM_LIB), SYM_EVPUSH);
    __sync_lock_release(&resolving);
}

__asm__(".text\n.arm\n.align 2\n.global " SYM_EVPUSH "\n.type " SYM_EVPUSH ", %function\n" SYM_EVPUSH ":\n"
        "    push {r0-r3, r12, lr}\n"
        "    bl cxdoom_note_push\n"
        "    mov r12, r0\n"
        "    pop {r0-r3}\n"
        "    add sp, sp, #4\n"
        "    pop {lr}\n"
        "    bx r12\n"
        ".size " SYM_EVPUSH ", .-" SYM_EVPUSH "\n");

/* --- menú y turnos ------------------------------------------------------ */

void cxdoom_request(void)
{
    if (!cx_doom || state != IDLE) return;
    say("pedido", 0, 0);
    state = ARMED;
    armed_t = now();
    base_t = 0;
    step = 0;
}

/*
 * Antes de la openView original. 1: era la pantalla que lanza el juego y no se
 * abre. La vista que llama es el submenú; se cierra a sí misma igual que cuando
 * se toca su botón de cerrar, y el menú principal se cierra en un turno posterior.
 */
int cxdoom_open(u32 self, const char *name)
{
    int i;
    if (!cx_doom || !name) return 0;
    if (starts(name, "view/GLOBAL_MENU") || starts(name, "view/MOV_REC") || starts(name, "view/STL_REC") ||
        starts(name, "view/ROOT")) {
        in_sub = 0;                    /* empieza otra cadena de pantallas */
        return 0;
    }
    if (has(name, "_MENU")) {
        for (i = 0; i < (int)sizeof(sub_name) - 1 && name[i]; i++) sub_name[i] = name[i];
        sub_name[i] = 0;
        gm_self = self;                /* quien abre un submenú es el menú principal */
        in_sub = 1;
        return 0;
    }
    /* Sólo desde el submenú: la pantalla de conexión USB tiene un botón que abre la misma vista. */
    if (!in_sub || !cx_doom_view[0] || !starts(name, cx_doom_view)) return 0;
    say("pedido desde el menu", self, (u32)state);
    if (state == IDLE) {
        cxframe_prepare();             /* aquí, que es el toque en el ítem */
        cxdoom_request();
        step = 1;
        in_sub = 0;
        cxview_close(self, sub_name);
    }
    return 1;
}

/* Después de una closeView: si era el submenú o el menú principal, ya no hay submenú. */
void cxdoom_closed(const char *name)
{
    if (name && in_sub && (starts(name, "view/GLOBAL_MENU") || starts(name, sub_name))) in_sub = 0;
}

/* Un toque en la esquina de la pantalla de grabación: arranca ya, en este contexto. */
void cxdoom_touch_start(void)
{
    if (!cx_doom || state != IDLE) return;
    say("pedido desde la esquina", 0, 0);
    if (!cxframe_prepare()) { say("no hay cuadro", 0, 0); return; }
    start();
}

/* Desde el hilo de la interfaz, unas 80 veces por segundo en la pantalla de grabación. */
void cxdoom_tick(void)
{
    u32 t;
    cxsound_tick();
    if (state == IDLE) return;
    t = now();
    if (state == ARMED) {
        if (cxpanel_base()) {
            if (!base_t) base_t = t;
            else if (t - base_t >= 300) start();
        } else {
            base_t = 0;
            if (step == 1 && t - armed_t >= CX_DOOM_CLOSE_MS) {
                step = 2;
                cxview_close(gm_self, "view/GLOBAL_MENU");
            } else if (t - armed_t > 30000) {
                state = IDLE;          /* el menú siguió abierto: se olvida */
            }
        }
        return;
    }
    if (!cxpanel_base()) { stop("otra pantalla: sale del juego"); return; }
    if (g_ended) { g_ended = 0; stop("el juego termino o no arranco"); return; }
    while (ring_r != ring_w && state == RUNNING) button(ring[ring_r++ & (RING - 1)]);
    if (state != RUNNING) return;
    n_turn++;
    if (!panel_off && cx_doom_panel != PANEL_ALWAYS && t - run_t >= cx_doom_panel * 1000) {
        stats("fin de la fase con panel: turnos, cuadros");
        panel_off = 1;
        cxpanel_update();              /* desde aquí, sin panel, como en la v32 a la v35 */
        stats("panel oculto");
    }
    if (cxframe_front()) n_front++;
    if (mbox & 4) {                    /* un cuadro nuevo: se cambia por el que tenía la interfaz */
        ui_idx = __sync_lock_test_and_set(&mbox, ui_idx) & 3;
        __sync_synchronize();
        pending = 1;
    }
    if (pending && t - shown_t >= cx_doom_ms) {
        u32 row;
        u16 *p = cxframe_lock(&row, 0);
        if (!p) {
            n_busy++;                  /* lo está leyendo la pantalla: en el turno que sigue */
        } else {
            const u16 *src = fb[ui_idx];
            int x, y;
            for (y = 0; y < 200; y++, p += row, src += 320)
                for (x = 0; x < 320; x++) p[x] = src[x];
            cxframe_unlock(1);
            pending = 0;
            shown_t = t;
            n_frame++;
        }
    }
    if (t - stat_t >= 4000 && n_stat < 12) { stat_t = t; n_stat++; stats("turnos de la interfaz, cuadros mostrados"); }
}
