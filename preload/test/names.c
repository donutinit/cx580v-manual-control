/*
 * Saca los diccionarios número -> nombre de libObj.so (firmware 1.07) llamando
 * a sus funciones de depuración. Corre en QEMU. Sin libc.
 *
 * Esas funciones son código puro: comparan el número y devuelven la dirección
 * de un texto relativa al propio código. Basta mapear el segmento de código
 * del archivo (sin cargarlo como biblioteca, porque su inicialización exige
 * /dev/uipc) y llamarlas. Salida: "<tabla>\t<número hex>\t<nombre>".
 */
#include "../cx.h"

#define LIB "/usr/lib/libObj.so"
#define TEXT_SIZE 0x10d99f8   /* primer PT_LOAD (R E) */

static const struct { const char *name; u32 off, first, last; } jobs[] = {
    { "MID",     0xa7eca8, 0x0, 0x20000 },   /* FUN_00a8eca8: mensajes */
    { "PARAMID", 0xa7f844, 0x0, 0x8000 },    /* FUN_00a8f844: parámetros */
    { "VALUEID", 0xa801c0, 0x0, 0x8000 },    /* FUN_00a901c0: valores */
};

static char out[4096];
static int on;

static void flush(void) { if (on) cx_write(1, out, on); on = 0; }
static void put(const char *s) { while (*s) { if (on == sizeof(out)) flush(); out[on++] = *s++; } }
static void hex(u32 v)
{
    static const char hx[] = "0123456789abcdef";
    char b[11];
    int i;
    b[0] = '0'; b[1] = 'x';
    for (i = 0; i < 8; i++) b[2 + i] = hx[(v >> (28 - 4 * i)) & 15];
    b[10] = 0;
    put(b);
}

void _start(void)
{
    long fd = cx_open(LIB, O_RDONLY, 0);
    u32 base, j, i;
    if (fd < 0) { put("no se pudo abrir " LIB "\n"); flush(); sys3(248, 1, 0, 0); }
    base = (u32)sys6(SYS_mmap2, 0, TEXT_SIZE, 5 /* R|X */, 2 /* PRIVATE */, fd, 0);
    if (base > 0xfffff000u) { put("mmap falló\n"); flush(); sys3(248, 1, 0, 0); }
    for (j = 0; j < sizeof(jobs) / sizeof(jobs[0]); j++) {
        u32 (*fn)(u32) = (u32 (*)(u32))(base + jobs[j].off);
        for (i = jobs[j].first; i <= jobs[j].last; i++) {
            u32 p = fn(i);
            const char *s;
            int k;
            if (p < base || p >= base + TEXT_SIZE) continue;
            s = (const char *)p;
            for (k = 0; k < 96 && s[k] > 32 && s[k] < 127; k++) ;
            if (k < 3 || s[k] != 0) continue;
            put(jobs[j].name); put("\t"); hex(i); put("\t"); put(s); put("\n");
        }
    }
    flush();
    sys3(248, 0, 0, 0);
}
