/*
 * Efectos de sonido de DOOM por la ruta de los pitidos de la cámara.
 *
 * No hay un dispositivo de audio en Linux: el sonido lo hace el procesador AV.
 * Lo más cercano a reproducir lo que uno quiera son los pitidos del sistema
 * (docs/research/audio-y-archivos.md): la interfaz manda
 * ObjMsg(0x3410, 0x1120) con el número de pitido, y libObj.so abre en ese
 * momento /usr/share/avsystemsound/Cam_*.bin con fopen, lo copia con fread al
 * búfer del procesador AV y le pide reproducirlo.
 *
 * Aquí:
 *  - Se desvían fopen, fread, feof y fclose sólo para libObj.so, escribiendo
 *    su tabla de saltos (no tiene RELRO). Si se abre uno de los pitidos de la
 *    tabla `beep` y hay un efecto de DOOM esperando para él, se entrega el
 *    efecto; todo lo demás pasa a la libc.
 *  - El juego (su hilo) deja cada efecto, y cada trozo de música, ya en el
 *    formato de los pitidos, en una ranura, y en el turno siguiente del hilo de
 *    la interfaz se manda el pitido. El objeto al que se mandan se toma del primer pitido de
 *    fábrica que pase por IssueCommandAsync (cxobj.c).
 *
 * Formato de los pitidos (deducido de los 32 archivos de la cámara; sin
 * confirmar): bloques de 32 bytes, un byte de cabecera con la escala en los 4
 * bits bajos (muestra × 2^escala) y, en el bit 4, si el bloque son diferencias,
 * seguido de 31 muestras con signo. Tasa supuesta: 48 kHz. El juego escribe
 * bloques directos con escala 8: sus muestras de 8 bits sin pérdida.
 *
 * cxmode.txt:  doomsound 1       efectos de sonido en DOOM
 */
#include "cx.h"

#ifndef CX_SOUND_LIB
#define CX_SOUND_LIB "/libObj.so"
#endif
#ifndef CX_SOUND_LIBC
#define CX_SOUND_LIBC "/libsonyc.so"
#endif
#define CAT_SYSSND 0x3410
#define NSLOT 8
#define SLOT_BYTES 65536
#define NROT 11
#define NBEEP 11
/*
 * Lo más que se le entrega a un pitido. El lector de la cámara (FUN_0044be90)
 * copia de a 128 bytes hasta llenar su búfer, y si el archivo mide lo mismo o
 * más que el búfer su bucle no termina nunca: pide 0 bytes, recibe 0 y vuelve a
 * pedir, con el candado del sonido tomado, y la interfaz se traba esperándolo
 * (v39, 2026-11-27: una escala de 396 KB congeló la cámara). El búfer lo da el
 * procesador AV según el tipo de memoria del pitido; alcanza para el pitido de
 * arranque de fábrica, 36 480 bytes, del mismo tipo que los de la rotación.
 * Si aun así se llega al final del búfer, w_fread lo nota, cierra la lectura y
 * baja este tope.
 */
#define CAP_START 36480

int cx_sound;
/*
 * La cámara guarda los pitidos cargados en una caché de 9 lugares y desaloja el
 * menos usado (FUN_00432934 busca, FUN_00433110 escoge): pedir otra vez el
 * mismo número no vuelve a leer el archivo (v38: sólo 3 lecturas en 20
 * pedidos). Rotando entre 11 pitidos que la interfaz casi no usa, cada pedido
 * es uno que ya se desalojó y obliga a leer. Hasta apagar la cámara, esos
 * pitidos pueden sonar a DOOM.
 */
static const struct { u32 id; const char *file; } beep[NBEEP] = {
    { 0x12, "/Cam_Jet_Touch.bin" },          /* BEEP_ID_JET_TOUCH */
    { 0x13, "/Cam_Jet_Sent.bin" },           /* BEEP_ID_JET_SENT */
    { 0x14, "/Cam_Jet_Error.bin" },          /* BEEP_ID_JET_ERROR */
    { 0x1e, "/Cam_Event_Anime_1line.bin" },
    { 0x1f, "/Cam_Event_Anime_2line.bin" },
    { 0x20, "/Cam_Event_Anime_3line.bin" },
    { 0x21, "/Cam_Event_Anime_4line.bin" },
    { 0x19, "/Cam_Media_Chg.bin" },
    { 0x25, "/Cam_HoldEnter.bin" },
    { 0x0d, "/Cam_SelfTimer.bin" },
    { 0x00, "/Cam_StartUp.bin" },
};

enum { S_FREE, S_FILLING, S_QUEUED, S_SENT, S_READING };
struct slot { volatile int st; u32 len, pos, t, tq, seq, which, music; int ended; u8 *buf; };
static struct slot slot[NSLOT];
static u32 fake[NSLOT][4];                   /* los FILE* falsos: sus direcciones */
static volatile u32 beep_obj;
static u32 r_fn[4];                          /* fopen, fread, feof, fclose de la libc */
static int hooked, n_log, n_play, n_drop, n_sent, n_read, n_fail, n_music, n_full;
static u32 seq, next_which;
static u32 m_last, m_gap_min = 0xffffffffu, m_gap_max, m_wait_max, m_nsend, m_nlog;
static volatile u32 buf_cap = CAP_START;

/* La siguiente de la rotación. */
static u32 rot(void)
{
    u32 w = next_which;
    next_which = next_which + 1 == NROT ? 0 : next_which + 1;
    return w;
}

static void say(const char *what, u32 a, u32 b)
{
    struct line l;
    if (!cx_log_enabled() || n_log >= 120) return;
    n_log++;
    cxl_begin(&l, "SN");
    cxl_lit(&l, " ");
    cxl_lit(&l, what);
    cxl_lit(&l, " ");
    cxl_hex(&l, a);
    cxl_lit(&l, " ");
    cxl_hex(&l, b);
    cxl_end(&l);
}

static u32 ms(void)
{
    u32 f, s = cx_now(&f);
    return s * 954 + f;
}

static int fake_slot(u32 f)
{
    if (f < (u32)fake || f >= (u32)fake + sizeof(fake)) return -1;
    return (int)((f - (u32)fake) >> 4);
}

static int which_file(const char *p)
{
    int k, n = 0, m;
    while (p[n]) n++;
    for (k = 0; k < NBEEP; k++) {
        const char *s = beep[k].file;
        for (m = 0; s[m]; m++) ;
        if (n >= m) {
            int j;
            for (j = 0; j < m && p[n - m + j] == s[j]; j++) ;
            if (j == m) return k;
        }
    }
    return -1;
}

/* --- las cuatro funciones de archivo, desde el hilo de los pitidos --------- */

static u32 w_fopen(const char *path, const char *mode)
{
    int w = path ? which_file(path) : -1, i, best = -1;
    if (w >= 0) {
        for (i = 0; i < NSLOT; i++)
            if (slot[i].st == S_SENT && slot[i].which == (u32)w && (best < 0 || slot[i].seq < slot[best].seq)) best = i;
        if (best >= 0 && __sync_bool_compare_and_swap(&slot[best].st, S_SENT, S_READING)) {
            slot[best].pos = 0;
            slot[best].ended = 0;
            if (n_read < 6) say("entrego (ranura, bytes)", (u32)best, slot[best].len);
            n_read++;
            return (u32)fake[best];
        }
    }
    return ((u32 (*)(const char *, const char *))r_fn[0])(path, mode);
}

/* El búfer de la cámara se acabó en `at` bytes: de ahí en adelante, menos. */
static void learn_cap(u32 at)
{
    u32 c = (at - 1) & ~31u;
    n_full++;
    if (c < buf_cap) buf_cap = c;
    say("el bufer de la camara se lleno en (bytes, tope nuevo)", at, buf_cap);
}

static u32 w_fread(void *p, u32 size, u32 count, u32 f)
{
    int i = fake_slot(f);
    u32 n, avail, k;
    if (i < 0) return ((u32 (*)(void *, u32, u32, u32))r_fn[1])(p, size, count, f);
    if (slot[i].st != S_READING || slot[i].ended || !size) return 0;
    n = size * count;
    avail = slot[i].len - slot[i].pos;
    if (size == 1 && n == 0 && avail) {
        /* El lector llegó justo al final de su búfer y volvería a pedir 0 bytes
         * para siempre: un byte «de más» (no se escribe nada) lo saca del bucle. */
        learn_cap(slot[i].pos);
        slot[i].ended = 1;
        return 1;
    }
    if (size == 1 && n < 128 && avail > n) {
        /* Pidió menos de 128: le queda ese tanto de búfer y a nosotros más. Se le da
         * uno menos y feof dice que se acabó, así su bucle termina limpio. */
        learn_cap(slot[i].pos + n);
        n--;
        slot[i].ended = 1;
    }
    if (n > avail) n = avail;
    if (size != 1) {                   /* el lector usa size 1; por si acaso, elementos enteros */
        u32 whole = 0;
        while ((whole + 1) * size <= n) whole++;
        n = whole * size;
    }
    for (k = 0; k < n; k++) ((u8 *)p)[k] = slot[i].buf[slot[i].pos + k];
    slot[i].pos += n;
    if (size == 1) return n;
    for (k = 0; k * size < n; k++) ;
    return k;
}

static int w_feof(u32 f)
{
    int i = fake_slot(f);
    if (i < 0) return ((int (*)(u32))r_fn[2])(f);
    return slot[i].ended || slot[i].pos >= slot[i].len;
}

static int w_fclose(u32 f)
{
    int i = fake_slot(f);
    if (i < 0) return ((int (*)(u32))r_fn[3])(f);
    if (n_read <= 6 || slot[i].pos < slot[i].len) say("cerrado (bytes leidos, de)", slot[i].pos, slot[i].len);
    __sync_bool_compare_and_swap(&slot[i].st, S_READING, S_FREE);
    return 0;
}

/* --- desde el juego y desde la interfaz ------------------------------------ */

/* El objeto de los pitidos: lo trae el primer pitido de fábrica (cxobj.c). */
void cxsound_note(u32 obj, u32 cat, u32 mid)
{
    if (cat != CAT_SYSSND || !obj || beep_obj == obj) return;
    beep_obj = obj;
    say("objeto de los pitidos (msg)", obj, mid);
}

static int alloc(void)
{
    long p;
    int i;
    if (slot[0].buf) return 1;
    p = sys6(SYS_mmap2, 0, NSLOT * SLOT_BYTES, 3, 0x22, -1, 0);
    if ((u32)p > 0xfffff000u) { say("sin memoria para los efectos", (u32)p, 0); return 0; }
    for (i = 0; i < NSLOT; i++) slot[i].buf = (u8 *)p + i * SLOT_BYTES;
    return 1;
}

/*
 * Deja `data` en una ranura para el siguiente turno de la interfaz (hilo del
 * juego). La música tiene reservadas las dos últimas ranuras, para que los
 * efectos no le quiten el lugar.
 */
static void put(const u8 *data, u32 len, int music)
{
    int i, from = music ? NSLOT - 2 : 0, to = music ? NSLOT : NSLOT - 2;
    u32 k, cap = buf_cap;
    if (!hooked || !len || !alloc()) return;
    if (len > cap) len = cap;
    len &= ~31u;
    for (i = from; i < to; i++)
        if (__sync_bool_compare_and_swap(&slot[i].st, S_FREE, S_FILLING)) break;
    if (i == to) { n_drop++; return; }
    for (k = 0; k < len; k += 4) *(u32 *)(slot[i].buf + k) = *(const u32 *)(data + k);
    slot[i].len = len;
    slot[i].which = next_which;
    slot[i].music = (u32)music;
    slot[i].tq = ms();
    next_which = next_which + 1 == NROT ? 0 : next_which + 1;
    __sync_synchronize();
    slot[i].st = S_QUEUED;
}

/* Un efecto ya en el formato de los pitidos (hilo del juego). */
void cxsound_play(const u8 *data, u32 len) { n_play++; put(data, len, 0); }

/* Un trozo de música, igual que un efecto pero en sus propias ranuras. */
void cxsound_music(const u8 *data, u32 len) { n_music++; put(data, len, 1); }

/* El tope actual de bytes por pitido. */
u32 cxsound_cap(void) { return buf_cap; }

/* En cada turno del hilo de la interfaz: manda lo que espera y libera lo vencido. */
void cxsound_tick(void)
{
    u32 t;
    int i;
    if (!hooked) return;
    t = ms();
    for (i = 0; i < NSLOT; i++) {
        int st = slot[i].st;
        if (st == S_QUEUED) {
            u32 r;
            if (!beep_obj) {           /* todavía no pasó ningún pitido de fábrica */
                slot[i].st = S_FREE;
                if (n_drop++ < 2) say("sin objeto de pitidos: toca algo del menu antes", 0, 0);
                continue;
            }
            slot[i].seq = ++seq;
            slot[i].t = t;
            slot[i].st = S_SENT;       /* antes de mandar: el lector puede abrir enseguida */
            r = cxobj_beep(beep_obj, beep[slot[i].which].id);
            n_sent++;
            if (slot[i].music) {               /* cada cuánto salen de verdad los trozos */
                u32 gap = m_last ? t - m_last : 0, wait = t - slot[i].tq;
                m_last = t;
                if (gap && gap < m_gap_min) m_gap_min = gap;
                if (gap > m_gap_max) m_gap_max = gap;
                if (wait > m_wait_max) m_wait_max = wait;
                if (++m_nsend == 16) {
                    if (m_nlog++ < 6) say("musica: entre envios (min, max) ms; espera max ms",
                                          (m_gap_min << 16) | m_gap_max, m_wait_max);
                    m_nsend = 0;
                    m_gap_min = 0xffffffffu;
                    m_gap_max = m_wait_max = 0;
                }
            }
            if (r) {
                n_fail++;
                __sync_bool_compare_and_swap(&slot[i].st, S_SENT, S_FREE);
                say("el pitido no salio (id, ret)", beep[slot[i].which].id, r);
            } else if (n_sent <= 4) {
                say(slot[i].music ? "musica mandada (id, bytes)" : "pitido mandado (id, bytes)", beep[slot[i].which].id, slot[i].len);
            }
        } else if ((st == S_SENT || st == S_READING) && t - slot[i].t > 3000) {
            if (__sync_bool_compare_and_swap(&slot[i].st, st, S_FREE)) say("vencio sin leerse (ranura, estado)", (u32)i, (u32)st);
        }
    }
}

void cxsound_stats(void)
{
    say("efectos, trozos de musica", (u32)n_play, (u32)n_music);
    say("mandados, leidos, fallidos", (u32)n_sent, ((u32)n_read << 16) | (u32)n_fail);
    say("perdidos, bufer lleno, tope", ((u32)n_drop << 16) | (u32)n_full, buf_cap);
}

/* Al cargar la biblioteca en im.elf: desvía las cuatro funciones de libObj.so. */
void cxsound_start(void)
{
    static const char *const name[4] = { "fopen", "fread", "feof", "fclose" };
    const u32 mine[4] = { (u32)w_fopen, (u32)w_fread, (u32)w_feof, (u32)w_fclose };
    u32 *got[4], lib, libc;
    int i;
    if (!cx_sound || hooked) return;
    lib = cx_lib_base(CX_SOUND_LIB);
    libc = cx_lib_base(CX_SOUND_LIBC);
    for (i = 0; i < 4; i++) {
        got[i] = cx_got_slot(lib, name[i]);
        r_fn[i] = cx_dlsym(libc, name[i]);
        if (!got[i] || !r_fn[i]) { say("no se pudo desviar", (u32)i, (u32)got[i]); return; }
    }
    for (i = 0; i < 4; i++) *got[i] = mine[i];
    hooked = 1;
    say("desviadas fopen y fread de libObj en", (u32)got[0], lib);
}
