/*
 * cxdl: busca bibliotecas cargadas y sus símbolos, sin libc.
 *
 * cx_lib_base() lee /proc/self/maps y devuelve dónde está cargada una
 * biblioteca (el mapeo r-x con desplazamiento 0). cx_dlsym() busca un símbolo
 * en la tabla dinámica de esa biblioteca en memoria, con su tabla hash SysV
 * (todas las de la cámara la traen).
 */

#include "cx.h"

static int hexval(char c)
{
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    return -1;
}

static int ends_with(const char *s, int n, const char *suffix)
{
    int sl = 0, j;
    while (suffix[sl]) sl++;
    if (n < sl) return 0;
    for (j = 0; j < sl; j++)
        if (s[n - sl + j] != suffix[j]) return 0;
    return 1;
}

u32 cx_lib_base(const char *suffix)
{
    static char chunk[1024];
    char line[256];
    int ll = 0;
    long fd = cx_open("/proc/self/maps", O_RDONLY, 0), n, i;
    u32 found = 0;
    if (fd < 0) return 0;
    while (!found && (n = cx_read(fd, chunk, sizeof(chunk))) > 0) {
        for (i = 0; i < n && !found; i++) {
            char c = chunk[i];
            if (c != '\n') {
                if (ll < (int)sizeof(line)) line[ll++] = c;
                continue;
            }
            /* "start-end perms offset dev inode path" */
            if (ends_with(line, ll, suffix)) {
                u32 start = 0, off = 0;
                int k = 0, v;
                while (k < ll && (v = hexval(line[k])) >= 0) { start = (start << 4) | v; k++; }
                while (k < ll && line[k] != ' ') k++;        /* -end */
                k++;
                if (k + 4 <= ll && line[k] == 'r' && line[k + 2] == 'x') {
                    k += 5;
                    while (k < ll && (v = hexval(line[k])) >= 0) { off = (off << 4) | v; k++; }
                    if (off == 0) found = start;
                }
            }
            ll = 0;
        }
    }
    cx_close(fd);
    return found;
}

static u32 elf_hash(const char *name)
{
    u32 h = 0, g;
    while (*name) {
        h = (h << 4) + (u8)*name++;
        g = h & 0xf0000000;
        if (g) h ^= g >> 24;
        h &= ~g;
    }
    return h;
}

/* a % b sin libgcc (ARMv6 no divide por hardware). */
static u32 umod(u32 a, u32 b)
{
    u32 bit = 1, d = b;
    if (!b) return 0;
    while (d < a && !(d & 0x80000000)) { d <<= 1; bit <<= 1; }
    while (bit) {
        if (a >= d) a -= d;
        d >>= 1;
        bit >>= 1;
    }
    return a;
}

static int streq(const char *a, const char *b)
{
    while (*a && *a == *b) { a++; b++; }
    return *a == *b;
}

/* Dirección de un símbolo definido en la biblioteca cargada en base, o 0. */
u32 cx_dlsym(u32 base, const char *name)
{
    const u8 *eh = (const u8 *)base;
    u32 phoff, i, h, idx;
    u16 phnum;
    const u32 *dyn = 0, *hash = 0, *bucket, *chain;
    const u8 *symtab = 0;
    const char *strtab = 0;

    if (!base || eh[0] != 0x7f || eh[1] != 'E' || eh[2] != 'L' || eh[3] != 'F') return 0;
    phoff = *(const u32 *)(eh + 0x1c);
    phnum = *(const u16 *)(eh + 0x2c);
    for (i = 0; i < phnum; i++) {
        const u32 *ph = (const u32 *)(eh + phoff + 32 * i);
        if (ph[0] == 2 /* PT_DYNAMIC */) dyn = (const u32 *)(base + ph[2]);
    }
    if (!dyn) return 0;
    for (; dyn[0]; dyn += 2) {
        u32 v = dyn[1] < base ? dyn[1] + base : dyn[1];   /* sin reubicar o reubicado */
        if (dyn[0] == 4) hash = (const u32 *)v;           /* DT_HASH */
        else if (dyn[0] == 5) strtab = (const char *)v;   /* DT_STRTAB */
        else if (dyn[0] == 6) symtab = (const u8 *)v;     /* DT_SYMTAB */
    }
    if (!hash || !strtab || !symtab || !hash[0]) return 0;
    bucket = hash + 2;
    chain = bucket + hash[0];
    h = umod(elf_hash(name), hash[0]);
    for (idx = bucket[h]; idx && idx < hash[1]; idx = chain[idx]) {
        const u8 *s = symtab + 16 * idx;
        u32 st_name = *(const u32 *)s, st_value = *(const u32 *)(s + 4);
        u16 shndx = *(const u16 *)(s + 14);
        if (shndx && st_value && streq(strtab + st_name, name)) return base + st_value;
    }
    return 0;
}
