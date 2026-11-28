/*
 * Prueba de CXDOOM.BIN fuera de la cámara (QEMU): hace de anfitrión igual que
 * preload/cxdoom.c. Carga la imagen en su dirección, arranca el juego, corre
 * cuadros a 35 por segundo, manda unas teclas y vuelca cuadros a archivos
 * crudos (328x200 de 16 bits) para verlos.
 *
 *   host CXDOOM.BIN doom1.wad DIR_SALIDA
 */
#include "../../preload/cxd.h"

typedef unsigned u32;
typedef unsigned short u16;

static long sys(long n, long a, long b, long c, long d, long e, long f)
{
    register long r7 __asm__("r7") = n;
    register long r0 __asm__("r0") = a;
    register long r1 __asm__("r1") = b;
    register long r2 __asm__("r2") = c;
    register long r3 __asm__("r3") = d;
    register long r4 __asm__("r4") = e;
    register long r5 __asm__("r5") = f;
    __asm__ volatile("svc 0" : "+r"(r0) : "r"(r7), "r"(r1), "r"(r2), "r"(r3), "r"(r4), "r"(r5) : "memory");
    return r0;
}

static u32 len(const char *s) { u32 n = 0; while (s[n]) n++; return n; }
static void say(const char *s) { sys(4, 1, (long)s, len(s), 0, 0, 0); }
static void log_line(const char *s) { say("  | "); say(s); say("\n"); }
static u32 n_mus, mus_bytes, mus_bad;
static long mus_fd = -1;
/*
 * Los trozos de música, mezclados como en la cámara: el trozo k empieza en k
 * segundos (24 000 muestras) y sus primeras 3 600 (150 ms) se suman con las
 * últimas del anterior. Al final, mix va a music.raw (16 bits a 24 kHz).
 */
#define MIX_LEN (24000 * 40)
static int mix[MIX_LEN];
static void music(const unsigned char *d, unsigned len)
{
    u32 i, k, n = 0, at = n_mus * 24000;
    n_mus++;
    mus_bytes += len;
    if (!len || (len & 31)) mus_bad++;
    for (i = 0; i < len; i += 32) {
        if (d[i] > 8) { mus_bad++; break; }
        for (k = 1; k < 32; k++, n++)
            if (at + n < MIX_LEN) mix[at + n] += (signed char)d[i + k] * (1 << d[i]);
    }
}
static void music_flush(void)
{
    static short pcm[4096];
    u32 i, n = 0, end = n_mus * 24000 + 3600;
    if (end > MIX_LEN) end = MIX_LEN;
    for (i = 0; i < end; i++) {
        int v = mix[i];
        pcm[n++] = (short)(v > 32767 ? 32767 : v < -32768 ? -32768 : v);
        if (n == 4096) { if (mus_fd >= 0) sys(4, mus_fd, (long)pcm, n * 2, 0, 0, 0); n = 0; }
    }
    if (n && mus_fd >= 0) sys(4, mus_fd, (long)pcm, n * 2, 0, 0, 0);
}
static u32 n_snd, snd_bytes, snd_bad, snd_first;
/* Un efecto en el formato de los pitidos: bloques de 32 bytes con la escala 8 en la cabecera. */
static void sound(const unsigned char *d, unsigned len)
{
    u32 i;
    n_snd++;
    snd_bytes += len;
    if (!snd_first) snd_first = len;
    if (!len || (len & 31)) snd_bad++;
    for (i = 0; i < len; i += 32)
        if (d[i] != 8) { snd_bad++; break; }
}
static void num(u32 v) { char b[12]; int i = 11; b[i] = 0; do { b[--i] = '0' + v % 10; v /= 10; } while (v); say(b + i); }
static u32 now_ms(void) { long ts[2]; sys(263, 1, (long)ts, 0, 0, 0, 0); return (u32)ts[0] * 1000u + (u32)ts[1] / 1000000u; }
static void sleep_ms(u32 ms) { long ts[2] = { 0, (long)ms * 1000000 }; sys(162, (long)ts, 0, 0, 0, 0, 0); }

#define PITCH 328
static u16 fb[PITCH * 200];
static char path[200];

static void dump(const char *dir, const char *name)
{
    u32 i = 0, k;
    long fd;
    for (k = 0; dir[k]; k++) path[i++] = dir[k];
    path[i++] = '/';
    for (k = 0; name[k]; k++) path[i++] = name[k];
    path[i] = 0;
    fd = sys(5, (long)path, 0x241 /* O_WRONLY|O_CREAT|O_TRUNC */, 0644, 0, 0, 0);
    if (fd < 0) { say("no pude crear "); say(path); say("\n"); return; }
    sys(4, fd, (long)fb, sizeof(fb), 0, 0, 0);
    sys(6, fd, 0, 0, 0, 0, 0);
}

int main_c(int argc, char **argv)
{
    struct cxd_header h;
    struct cxd_host host = { log_line, 0, 6, { 12, 8, 4 }, 0, 0x000f, sound, 24000, 8, music, 24000, 1000, 1150, 1 };
    long fd, got, p;
    u32 sum = 0, i, t0, frames = 0, worst = 0;
    int r, tick;
    if (argc < 4) { say("uso: host CXDOOM.BIN doom1.wad DIR\n"); return 2; }
    host.wad = argv[2];
    fd = sys(5, (long)argv[1], 0, 0, 0, 0, 0);
    if (fd < 0 || sys(3, fd, (long)&h, sizeof(h), 0, 0, 0) != sizeof(h)) { say("no pude leer la imagen\n"); return 1; }
    if (h.magic != CXD_MAGIC || h.version != CXD_VERSION || h.nreloc > (h.mem_size - h.file_size) / 4) { say("cabecera mala\n"); return 1; }
    p = sys(192, 0, h.mem_size, 7, 0x22, -1, 0);                  /* donde haya sitio, como en la cámara */
    if ((u32)p > 0xfffff000u || (u32)p == h.base) { say("sin memoria, o cayo justo en la direccion de enlace\n"); return 1; }
    sys(19, fd, 0, 0, 0, 0, 0);
    for (got = 0; got < (long)(h.file_size + h.nreloc * 4);) {
        long n = sys(3, fd, p + got, h.file_size + h.nreloc * 4 - got, 0, 0, 0);
        if (n <= 0) { say("lectura corta\n"); return 1; }
        got += n;
    }
    sys(6, fd, 0, 0, 0, 0, 0);
    for (i = 64; i < h.file_size + h.nreloc * 4; i += 4) sum += *(const u32 *)(p + i);
    if (sum != h.sum) { say("suma mala\n"); return 1; }
    for (i = 0; i < h.nreloc; i++) {
        u32 *slot = (u32 *)(p + h.file_size + i * 4);
        *(u32 *)(p + *slot) += (u32)p - h.base;
        *slot = 0;
    }
    sys(0xf0002, p, p + h.file_size, 0, 0, 0, 0);                 /* cacheflush */
    h = *(const struct cxd_header *)p;

    {   /* la música decodificada, para revisarla después */
        u32 i2 = 0, k2;
        for (k2 = 0; argv[3][k2]; k2++) path[i2++] = argv[3][k2];
        for (k2 = 0; "/music.raw"[k2]; k2++) path[i2++] = "/music.raw"[k2];
        path[i2] = 0;
        mus_fd = sys(5, (long)path, 0x241, 0644, 0, 0, 0);
    }
    t0 = now_ms();
    r = ((cxd_init_fn)h.init)(&host);
    say("init = "); num((u32)r); say(", ms "); num(now_ms() - t0); say("\n");
    if (r) return 1;
    for (tick = 0; tick < 700; tick++) {
        u32 a = now_ms(), d;
        if (tick == 330) ((cxd_key_fn)h.key)(1, CXK_ESC);          /* abre el menú */
        if (tick == 332) ((cxd_key_fn)h.key)(0, CXK_ESC);
        if (tick == 360 || tick == 380 || tick == 400) ((cxd_key_fn)h.key)(1, CXK_ENTER);   /* New Game, episodio, dificultad */
        if (tick == 362 || tick == 382 || tick == 402) ((cxd_key_fn)h.key)(0, CXK_ENTER);
        if (tick == 470) ((cxd_key_fn)h.key)(1, CXK_UP);
        if (tick == 520) ((cxd_key_fn)h.key)(1, CXK_FIRE);
        if (tick == 540) { ((cxd_key_fn)h.key)(0, CXK_FIRE); ((cxd_key_fn)h.key)(0, CXK_UP); }
        r = ((cxd_tick_fn)h.tick)(fb, PITCH);
        if (r >= 0) ((cxd_music_fn)h.music)();          /* en la cámara va en otro hilo */
        d = now_ms() - a;
        if (d > worst) worst = d;
        if (r < 0) { say("el juego termino en el cuadro "); num((u32)tick); say("\n"); return 1; }
        frames += (u32)r;
        if (tick == 20) dump(argv[3], "f020.raw");
        if (tick == 300) dump(argv[3], "f300.raw");
        if (tick == 350) dump(argv[3], "f350.raw");
        if (tick == 460) dump(argv[3], "f460.raw");
        if (tick == 535) dump(argv[3], "f535.raw");
        if (tick == 699) dump(argv[3], "f699.raw");
        if (d < 29) sleep_ms(29 - d);
    }
    say("cuadros pintados "); num(frames); say(" de 700, el mas lento ms "); num(worst); say("\n");
    say("efectos "); num(n_snd); say(", bytes "); num(snd_bytes); say(", el primero "); num(snd_first);
    say(", mal formados "); num(snd_bad); say("\n");
    if (!n_snd || snd_bad) { say("los efectos no salieron bien\n"); return 1; }
    music_flush();
    say("musica: trozos "); num(n_mus); say(", bytes "); num(mus_bytes); say(", mal formados "); num(mus_bad); say("\n");
    if (!n_mus || mus_bad) { say("la musica no salio bien\n"); return 1; }
    /* Lo que sobra fuera de 320 columnas no se toca. */
    for (i = 0; i < 200; i++)
        if (fb[i * PITCH + 320] || fb[i * PITCH + 327]) { say("escribio fuera del cuadro\n"); return 1; }
    say("HOST OK\n");
    return 0;
}

__asm__(".text\n.arm\n.global _start\n_start:\n"
        "    ldr r0, [sp]\n    add r1, sp, #4\n    bl main_c\n    mov r7, #1\n    svc 0\n");
