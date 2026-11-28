/*
 * DOOM en la HDR-CX580V: la capa de plataforma de doomgeneric.
 *
 * El anfitrión (preload/cxdoom.c, dentro de cxg.so) carga esta imagen, llama a
 * init una vez y luego a tick desde el hilo de la interfaz de la cámara, con el
 * bitmap de la pantalla ya bloqueado. tick avanza el juego lo que haya pasado
 * de tiempo y, si el motor dibujó, deja el cuadro de 320x200 convertido al
 * formato de 16 bits del OSD. Las teclas llegan por key.
 *
 * Nada aquí bloquea más de lo que tarda un cuadro, salvo el arranque (leer el
 * WAD y preparar las texturas): el fundido entre pantallas, que en el original
 * es un bucle de un segundo, se quita en la compilación (doom/build.sh).
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "doomgeneric.h"
#include "doomkeys.h"
#include "doomtype.h"
#include "i_system.h"
#include "i_video.h"
#include "m_controls.h"
#include "cxd.h"
#include "cxlibc.h"

#define SYS_munmap 91
#define SYS_nanosleep 162
#define SYS_mmap2 192
#define SYS_clock_gettime 263

static const struct cxd_host *host;

/* Música (al final del archivo). */
extern unsigned int opl_sample_rate;
extern int snd_samplerate;
static unsigned m_have, m_chunks, m_t0, m_sent, m_quiet, m_stat, m_cost;
static int title_music;
static void music_pump(void);
static void title_hold(void);
static volatile int mlock, music_ready, music_dead;
static unsigned tick_n, tick_logs;
static unsigned jb[10];
static int in_call, dead = 1, lines_left = 60;
static uint16_t lut[256];
static uint16_t *out;
static unsigned out_pitch;
static int have_frame;
static long arena_p, arena_n;        /* la memoria del juego, para devolverla al terminar */

#define NKEY 32                       /* potencia de 2 */
static struct { unsigned char pressed, key; } keys[NKEY];
static unsigned key_w, key_r;

/* Candado entre el hilo del juego y el de música (núcleos distintos: se cede la vez). */
static void mus_lock(void)
{
    while (__sync_lock_test_and_set(&mlock, 1)) cxd_sys(158 /* sched_yield */, 0, 0, 0, 0, 0, 0);
}
static void mus_unlock(void) { __sync_lock_release(&mlock); }

void cxd_log_line(const char *line)
{
    if (!host || !host->log) return;
    if (lines_left <= 0) return;
    lines_left--;
    host->log(line);
}

void cxd_exit(int code)
{
    char b[40];
    lines_left = 4;
    snprintf(b, sizeof(b), "DOOM termino (%d)", code);
    cxd_out("\n", 1);
    cxd_log_line(b);
    dead = 1;
    if (in_call) cxd_longjmp(jb, 1);
    for (;;) ;                        /* no debería pasar: exit fuera de una entrada */
}

/* --- lo que doomgeneric le pide a la plataforma ------------------------ */

void DG_Init(void) {}
void DG_SetWindowTitle(const char *title) { (void)title; }

void DG_DrawFrame(void)
{
    const uint8_t *s = (const uint8_t *)DG_ScreenBuffer;
    uint16_t *d = out;
    int x, y;
    if (!d || !s) return;
    if (palette_changed) {
        for (x = 0; x < 256; x++)
            lut[x] = (uint16_t)(((colors[x].r >> 4) << host->shift[0]) | ((colors[x].g >> 4) << host->shift[1]) |
                                ((colors[x].b >> 4) << host->shift[2]) | host->opaque);
        palette_changed = false;
    }
    for (y = 0; y < DOOMGENERIC_RESY; y++, d += out_pitch, s += DOOMGENERIC_RESX)
        for (x = 0; x < DOOMGENERIC_RESX; x++) d[x] = lut[s[x]];
    have_frame = 1;
}

void DG_SleepMs(uint32_t ms)
{
    long ts[2];
    ts[0] = 0;
    ts[1] = (long)ms * 1000000;
    cxd_sys(SYS_nanosleep, (long)ts, 0, 0, 0, 0, 0);
}

uint32_t DG_GetTicksMs(void)
{
    static long sec0;
    long ts[2] = { 0, 0 };
    cxd_sys(SYS_clock_gettime, 1 /* CLOCK_MONOTONIC */, (long)ts, 0, 0, 0, 0);
    if (!sec0) sec0 = ts[0] - 1;
    return (uint32_t)(ts[0] - sec0) * 1000u + (uint32_t)ts[1] / 1000000u;
}

int DG_GetKey(int *pressed, unsigned char *key)
{
    if (key_r == key_w) return 0;
    *pressed = keys[key_r & (NKEY - 1)].pressed;
    *key = keys[key_r & (NKEY - 1)].key;
    key_r++;
    return 1;
}

/* El último de la lista de salida del motor: «Quit Game» termina aquí. */
static void quit_hook(void) { exit(0); }

/* --- entradas (start.S las llama ya en la pila propia) ------------------ */

int cxd_init_c(const struct cxd_host *h)
{
    static char mb[8], wad[128];
    static char *argv[] = { "doom", "-iwad", wad, "-mb", mb, "-nomusic", 0 };
    unsigned zone = h->zone_mb ? h->zone_mb : 6;
    size_t arena = ((size_t)zone << 20) + (1u << 20);
    long p;

    host = h;
    lines_left = 60;
    p = cxd_sys(SYS_mmap2, 0, (long)arena, 3, 0x22, -1, 0);
    if ((unsigned long)p > 0xfffff000ul) { cxd_log_line("DOOM: no hay memoria para el juego"); return -1; }
    cxd_heap_init((void *)p, arena);
    arena_p = p;
    arena_n = (long)arena;
    snprintf(mb, sizeof(mb), "%u", zone);
    strncpy(wad, h->wad, sizeof(wad) - 1);
    key_nextweapon = CXK_NEXTW;
    palette_changed = true;

    in_call = 1;
    dead = 0;
    if (cxd_setjmp(jb)) {
        in_call = 0;
        cxd_sys(SYS_munmap, p, (long)arena, 0, 0, 0, 0);
        return -1;
    }
    I_AtExit(quit_hook, true);
    /* i_oplmusic.c fija la tasa del emulador con snd_samplerate (44100 de fábrica)
     * al iniciar la música: tiene que ser la misma con la que se leen las muestras.
     * Con 44100 contra 24000 la música salía casi una octava más grave y más lenta
     * (v40, el dueño lo notó el 2026-11-27). */
    snd_samplerate = (int)(h->music_rate ? h->music_rate : 24000);
    opl_sample_rate = (unsigned)snd_samplerate;
    m_t0 = m_have = m_chunks = m_sent = m_quiet = m_stat = m_cost = 0;
    tick_n = tick_logs = 0;
    title_music = 0;
    music_dead = 0;
    music_ready = 0;
    doomgeneric_Create(h->music ? 5 : 6, argv);
    music_ready = h->music ? 1 : 0;
    in_call = 0;
    return 0;
}

int cxd_tick_c(uint16_t *dst, unsigned pitch)
{
    if (dead) return -1;
    in_call = 1;
    if (cxd_setjmp(jb)) {              /* terminó («Quit Game» o un error): devuelve su memoria */
        in_call = 0;
        out = 0;
        mus_lock();                    /* que el hilo de música no esté generando */
        music_dead = 1;
        mus_unlock();
        if (arena_p) cxd_sys(SYS_munmap, arena_p, arena_n, 0, 0, 0, 0);
        arena_p = 0;
        return -1;
    }
    out = dst;
    out_pitch = pitch;
    have_frame = 0;
    {
        unsigned t0 = DG_GetTicksMs(), dt;
        title_hold();
        doomgeneric_Tick();
        dt = DG_GetTicksMs() - t0;
        tick_n++;
        if (tick_logs < 12 && (tick_n <= 4 || dt > 300)) {   /* los primeros turnos y los muy largos */
            char line[64];
            tick_logs++;
            snprintf(line, sizeof(line), "turno %u: %u ms", tick_n, dt);
            cxd_log_line(line);
        }
    }
    out = 0;
    in_call = 0;
    return have_frame;
}

void cxd_key_c(int pressed, int key)
{
    if (key_w - key_r >= NKEY) return;
    keys[key_w & (NKEY - 1)].pressed = (unsigned char)(pressed != 0);
    keys[key_w & (NKEY - 1)].key = (unsigned char)key;
    key_w++;
}

/* Los fundidos entre pantallas, sin el bucle que espera (ver doom/build.sh). */
int cxd_wipe_start(int x, int y, int w, int h) { (void)x; (void)y; (void)w; (void)h; return 0; }
int cxd_wipe_end(int x, int y, int w, int h) { (void)x; (void)y; (void)w; (void)h; return 0; }
int cxd_wipe(int kind, int x, int y, int w, int h, int tics)
{ (void)kind; (void)x; (void)y; (void)w; (void)h; (void)tics; return 1; }

/* --- sonido: efectos por la ruta de los pitidos de la cámara ------------ */
/*
 * Cada efecto del WAD (DMX: formato 3, tasa, largo y muestras de 8 bits sin
 * signo, con 16 bytes de relleno al principio y al final) se pasa a 48 kHz por
 * vecino más cercano y se escribe en bloques de pitido: un byte de cabecera con
 * la escala (sin el bit de diferencias) y 31 muestras con signo. Lo reproduce
 * la cámara; aquí no se sabe cuándo termina, así que ningún canal «sigue
 * sonando». Sin música.
 */
#include "i_sound.h"
#include "w_wad.h"
#include "z_zone.h"
#include "deh_str.h"

#define ENC_MAX 65536
static unsigned char enc[ENC_MAX];
static boolean sfx_prefix;

int use_libsamplerate = 0;
float libsamplerate_scale = 0.65f;
void I_InitTimidityConfig(void) {}

static boolean snd_init(boolean use_sfx_prefix)
{
    sfx_prefix = use_sfx_prefix;
    return host && host->sound && host->sound_hz;
}

static void snd_none(void) {}
static void snd_params(int channel, int vol, int sep) { (void)channel; (void)vol; (void)sep; }
static void snd_stop(int channel) { (void)channel; }
static boolean snd_playing(int channel) { (void)channel; return false; }
static void snd_cache(sfxinfo_t *sounds, int num_sounds) { (void)sounds; (void)num_sounds; }

static int snd_lump(sfxinfo_t *sfx)
{
    char name[9];
    if (sfx->link) sfx = sfx->link;
    snprintf(name, sizeof(name), sfx_prefix ? "ds%s" : "%s", DEH_String(sfx->name));
    return W_GetNumForName(name);
}

static int snd_start(sfxinfo_t *sfx, int channel, int vol, int sep)
{
    const unsigned char *d, *pcm;
    unsigned len, rate, n, out, i, b, pos = 0, hz = host->sound_hz;
    int lump = sfx->lumpnum;
    (void)sep;
    if (lump < 0) lump = snd_lump(sfx);
    if (lump < 0 || vol <= 0) return -1;
    d = W_CacheLumpNum(lump, PU_STATIC);
    len = W_LumpLength(lump);
    if (len < 8 || d[0] != 3 || d[1] != 0) { W_ReleaseLumpNum(lump); return -1; }
    rate = d[2] | (d[3] << 8);
    n = d[4] | (d[5] << 8) | ((unsigned)d[6] << 16) | ((unsigned)d[7] << 24);
    if (n > len - 8) n = len - 8;
    pcm = d + 8;
    if (n > 48) { pcm += 16; n -= 32; }      /* el relleno de DMX */
    if (!rate || !n) { W_ReleaseLumpNum(lump); return -1; }
    out = (unsigned)((unsigned long long)n * hz / rate);
    if (out > ENC_MAX / 32 * 31) out = ENC_MAX / 32 * 31;
    if (vol > 127) vol = 127;
    for (b = 0; b * 31 < out; b++) {
        unsigned char *blk = enc + b * 32;
        blk[0] = (unsigned char)(host->sound_shift & 15);
        for (i = 0; i < 31; i++, pos++) {
            int s = 0;
            if (pos < out) {
                unsigned src = (unsigned)((unsigned long long)pos * rate / hz);
                s = ((int)pcm[src < n ? src : n - 1] - 128) * vol / 127;
            }
            blk[1 + i] = (unsigned char)(signed char)s;
        }
    }
    W_ReleaseLumpNum(lump);
    host->sound(enc, b * 32);
    return channel;
}

static snddevice_t snd_devices[] = { SNDDEVICE_SB, SNDDEVICE_PAS, SNDDEVICE_GUS, SNDDEVICE_WAVEBLASTER,
                                     SNDDEVICE_SOUNDCANVAS, SNDDEVICE_AWE32 };

sound_module_t DG_sound_module = {
    snd_devices, sizeof(snd_devices) / sizeof(snd_devices[0]),
    snd_init, snd_none, snd_lump, snd_none, snd_params, snd_start, snd_stop, snd_playing, snd_cache,
};


/* --- música: OPL emulado (Chocolate Doom), en trozos por la ruta de los pitidos --- */
/*
 * El módulo de música es el de Chocolate Doom 2.2.1 (i_oplmusic.c, que se
 * compila como DG_music_module) con el emulador DBOPL; doom/opl_cxd.c genera las
 * muestras. Aquí se generan, en cada turno del juego, las que tocan según el
 * reloj, y cada `music_chunk_ms` se manda un trozo, pasado a la tasa de los
 * pitidos (sound_hz, que el anfitrión puede cambiar mientras se juega) y a bloques
 * de pitido con la escala justa para cada bloque. Los trozos en silencio no se
 * mandan.
 */
#include "doomstat.h"
#include "s_sound.h"
#include "sounds.h"

void cxd_opl_render(int16_t *out, unsigned int n);
extern int demosequence, pagetic;

#define MCHUNK_MAX (48000 * 4)       /* hasta 4 s */
#define MUSIC_GAIN 12
static int16_t mbuf[MCHUNK_MAX];     /* el trozo en curso, a la tasa del emulador */
static unsigned char menc[MCHUNK_MAX / 31 * 32 + 64];

/*
 * Manda mbuf[0, n). Con ov > 0, los primeros ov y los últimos ov muestras se
 * funden (entrada y salida): son los mismos que el final del trozo anterior y el
 * principio del siguiente, que suenan a la vez porque la cámara mezcla pitidos.
 */
static void music_send(unsigned n, unsigned ov)
{
    unsigned rate = host->music_rate, hz = host->sound_hz ? host->sound_hz : 24000;
    unsigned out = (unsigned)((unsigned long long)n * hz / rate);
    unsigned fo = (unsigned)((unsigned long long)ov * hz / rate);
    unsigned b, i, pos = 0, peak = 0;
    for (i = 0; i < n; i++) {
        int v = mbuf[i] < 0 ? -mbuf[i] : mbuf[i];
        if ((unsigned)v > peak) peak = (unsigned)v;
    }
    if (peak < 64) { m_quiet++; return; }         /* silencio: no se manda */
    for (b = 0; b * 31 < out; b++) {
        unsigned char *blk = menc + b * 32;
        int s[31], k, mx = 0, sh = 0;
        for (k = 0; k < 31; k++, pos++) {
            int v = 0;
            if (pos < out) {
                /* de la tasa del emulador a 48 kHz, interpolando entre vecinos */
                unsigned long long q = (unsigned long long)pos * rate;
                unsigned i0 = (unsigned)(q / hz), fr = (unsigned)(q % hz);
                int a = mbuf[i0 < n ? i0 : n - 1], c = mbuf[i0 + 1 < n ? i0 + 1 : n - 1];
                v = a + (int)((long long)(c - a) * fr / hz);
            }
            /* El OPL deja mucho margen: sin ganancia la música quedaba 22 dB por debajo
             * de los efectos (RMS 287 contra 3770 del sonido del menú; v43). */
            v *= MUSIC_GAIN;
            if (fo && pos < fo) v = (int)((long long)v * (int)pos / (int)fo);
            else if (fo && pos >= out - fo && pos < out) v = (int)((long long)v * (int)(out - pos) / (int)fo);
            if (v > 32767) v = 32767;
            if (v < -32768) v = -32768;
            s[k] = v;
            if (v < 0) v = -v;
            if (v > mx) mx = v;
        }
        while (sh < 8 && (mx >> sh) > 127) sh++;
        blk[0] = (unsigned char)sh;
        for (k = 0; k < 31; k++) {
            int v = sh ? (s[k] + (1 << (sh - 1))) >> sh : s[k];
            blk[1 + k] = (unsigned char)(signed char)(v > 127 ? 127 : v < -128 ? -128 : v);
        }
    }
    host->music(menc, b * 32);
    m_sent++;
}

/* En cada turno del juego: genera la música que corresponde a este momento. */
/*
 * En cada llamada (hilo de música): genera la música que toca según el reloj.
 * Cada trozo lleva `chunk` muestras nuevas precedidas por las últimas `ov` del
 * anterior (ov = music_lead_ms - music_chunk_ms), y sale `ov` antes de su hora:
 * así el retraso con que el hilo de la interfaz manda cada pitido cae dentro del
 * fundido y no deja un hueco (v44: cortes breves cada pocos segundos).
 */
static void music_pump(void)
{
    unsigned rate, chunk, ov = 0, now, want, done, budget;
    char line[112];
    if (!host->music || !host->music_rate) return;
    rate = host->music_rate;
    chunk = (unsigned)((unsigned long long)host->music_chunk_ms * rate / 1000);
    if (host->music_lead_ms > host->music_chunk_ms)
        ov = (unsigned)((unsigned long long)(host->music_lead_ms - host->music_chunk_ms) * rate / 1000);
    if (chunk < rate / 10) chunk = rate / 10;
    if (ov > chunk / 2) ov = chunk / 2;
    if (chunk + ov > MCHUNK_MAX) { chunk = MCHUNK_MAX * 2 / 3; if (ov > chunk / 2) ov = chunk / 2; }
    now = DG_GetTicksMs();
    if (!m_t0) { m_t0 = now; memset(mbuf, 0, ov * sizeof(mbuf[0])); }
    want = (unsigned)((unsigned long long)(now - m_t0 + host->music_lead_ms) * rate / 1000);
    done = m_chunks * chunk + m_have;
    if (want > done + 3 * chunk) {                /* venimos de una pausa: se salta lo perdido */
        m_chunks = want / chunk;
        m_have = 0;
        done = m_chunks * chunk;
        memset(mbuf, 0, ov * sizeof(mbuf[0]));
    }
    budget = rate / 4;                            /* como mucho 250 ms de música por llamada */
    while (done < want && budget) {
        unsigned k = want - done;
        if (k > chunk - m_have) k = chunk - m_have;
        if (k > budget) k = budget;
        {
            unsigned r0 = DG_GetTicksMs();
            cxd_opl_render(mbuf + ov + m_have, k);
            m_cost += DG_GetTicksMs() - r0;
        }
        m_have += k;
        done += k;
        budget -= k;
        if (m_have == chunk) {
            music_send(ov + chunk, ov);
            memmove(mbuf, mbuf + chunk, ov * sizeof(mbuf[0]));   /* el final, para el principio del siguiente */
            m_have = 0;
            m_chunks++;
            if (m_stat < 10 && (m_chunks <= 3 || !(m_chunks & 15))) {
                m_stat++;
                snprintf(line, sizeof(line), "musica: trozo %u (%u mandados, %u en silencio), %u ms de calculo para %u ms, solape %u",
                         m_chunks, m_sent, m_quiet, m_cost, host->music_chunk_ms, ov);
                cxd_log_line(line);
            }
            m_cost = 0;
        }
    }
}

/* La pantalla de título se queda y suena E1M1 en bucle (music_title). */
static void title_hold(void)
{
    if (!host->music_title) return;
    if (gamestate == GS_DEMOSCREEN && demosequence == 0) {
        if (pagetic < 35) pagetic = 35 * 60;
        if (!title_music) { title_music = 1; S_ChangeMusic(mus_e1m1, true); }
    } else {
        title_music = 0;
    }
}

/* --- el hilo de música ---------------------------------------------------- */

int cxd_music_c(void)
{
    int r = 0;
    mus_lock();
    if (music_dead) r = -1;
    else if (music_ready) music_pump();
    mus_unlock();
    return r;
}

/*
 * El módulo de música de Chocolate Doom se compila como cxd_opl_music; el que
 * ve el juego (DG_music_module) lo llama con el candado tomado, para no tocar el
 * secuenciador ni el chip mientras el otro hilo genera.
 */
extern music_module_t cxd_opl_music;
static boolean mw_init(void) { boolean r; mus_lock(); r = cxd_opl_music.Init(); mus_unlock(); return r; }
static void mw_shutdown(void) { mus_lock(); cxd_opl_music.Shutdown(); mus_unlock(); }
static void mw_volume(int v) { mus_lock(); cxd_opl_music.SetMusicVolume(v); mus_unlock(); }
static void mw_pause(void) { mus_lock(); cxd_opl_music.PauseMusic(); mus_unlock(); }
static void mw_resume(void) { mus_lock(); cxd_opl_music.ResumeMusic(); mus_unlock(); }
static void *mw_register(void *d, int len) { void *r; mus_lock(); r = cxd_opl_music.RegisterSong(d, len); mus_unlock(); return r; }
static void mw_unregister(void *h) { mus_lock(); cxd_opl_music.UnRegisterSong(h); mus_unlock(); }
static void mw_play(void *h, boolean loop) { mus_lock(); cxd_opl_music.PlaySong(h, loop); mus_unlock(); }
static void mw_stop(void) { mus_lock(); cxd_opl_music.StopSong(); mus_unlock(); }
static boolean mw_playing(void) { boolean r; mus_lock(); r = cxd_opl_music.MusicIsPlaying(); mus_unlock(); return r; }
static void mw_poll(void) { if (cxd_opl_music.Poll) { mus_lock(); cxd_opl_music.Poll(); mus_unlock(); } }

music_module_t DG_music_module = {
    snd_devices, sizeof(snd_devices) / sizeof(snd_devices[0]),
    mw_init, mw_shutdown, mw_volume, mw_pause, mw_resume, mw_register, mw_unregister, mw_play, mw_stop, mw_playing, mw_poll,
};
