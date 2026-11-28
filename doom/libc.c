/*
 * Biblioteca C mínima para correr doomgeneric dentro de im.elf, sin la libc de
 * la cámara: llamadas al sistema directas, un montón propio y lo justo de
 * stdio, string y printf que usa el motor.
 *
 * - Los archivos de verdad sólo se abren para leer. Escribir crea un archivo en
 *   memoria (lo usa la música para su MIDI temporal), que después se puede leer
 *   por el mismo nombre; nada se escribe en la flash. La configuración y las
 *   partidas guardadas no sobreviven al apagado.
 * - Lo que el motor imprime sale por líneas a la bitácora del anfitrión.
 * - exit() no termina el proceso (que es la interfaz de la cámara): vuelve a la
 *   entrada que estaba corriendo (cxd.c).
 */
#include <stddef.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include "cxlibc.h"

int errno;

long cxd_sys(long n, long a, long b, long c, long d, long e, long f)
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

#define SYS_read 3
#define SYS_open 5
#define SYS_close 6
#define SYS_lseek 19

/* --- memoria ---------------------------------------------------------- */

void *memcpy(void *dst, const void *src, size_t n)
{
    unsigned char *d = dst;
    const unsigned char *s = src;
    if ((((uintptr_t)d | (uintptr_t)s) & 3) == 0) {
        uint32_t *dw = dst;
        const uint32_t *sw = src;
        for (; n >= 4; n -= 4) *dw++ = *sw++;
        d = (unsigned char *)dw;
        s = (const unsigned char *)sw;
    }
    while (n--) *d++ = *s++;
    return dst;
}

void *memmove(void *dst, const void *src, size_t n)
{
    unsigned char *d = dst;
    const unsigned char *s = src;
    if (d <= s || d >= s + n) return memcpy(dst, src, n);
    while (n--) d[n] = s[n];
    return dst;
}

void *memset(void *dst, int c, size_t n)
{
    unsigned char *d = dst;
    for (; n && ((uintptr_t)d & 3); n--) *d++ = (unsigned char)c;
    if (n >= 4) {
        uint32_t w = (unsigned char)c * 0x01010101u, *dw = (uint32_t *)d;
        for (; n >= 4; n -= 4) *dw++ = w;
        d = (unsigned char *)dw;
    }
    while (n--) *d++ = (unsigned char)c;
    return dst;
}

int memcmp(const void *a, const void *b, size_t n)
{
    const unsigned char *x = a, *y = b;
    for (; n; n--, x++, y++)
        if (*x != *y) return *x - *y;
    return 0;
}

/* --- cadenas ---------------------------------------------------------- */

int isdigit(int c) { return c >= '0' && c <= '9'; }
int isspace(int c) { return c == ' ' || (c >= 9 && c <= 13); }
int isupper(int c) { return c >= 'A' && c <= 'Z'; }
int islower(int c) { return c >= 'a' && c <= 'z'; }
int isalpha(int c) { return isupper(c) || islower(c); }
int isalnum(int c) { return isalpha(c) || isdigit(c); }
int isprint(int c) { return c >= 32 && c < 127; }
int toupper(int c) { return islower(c) ? c - 32 : c; }
int tolower(int c) { return isupper(c) ? c + 32 : c; }

size_t strlen(const char *s) { size_t n = 0; while (s[n]) n++; return n; }
char *strcpy(char *d, const char *s) { char *r = d; while ((*d++ = *s++)) ; return r; }
char *strncpy(char *d, const char *s, size_t n)
{
    size_t i = 0;
    for (; i < n && s[i]; i++) d[i] = s[i];
    for (; i < n; i++) d[i] = 0;
    return d;
}
char *strcat(char *d, const char *s) { strcpy(d + strlen(d), s); return d; }
int strcmp(const char *a, const char *b)
{
    for (; *a && *a == *b; a++, b++) ;
    return (unsigned char)*a - (unsigned char)*b;
}
int strncmp(const char *a, const char *b, size_t n)
{
    for (; n && *a && *a == *b; a++, b++, n--) ;
    return n ? (unsigned char)*a - (unsigned char)*b : 0;
}
int strcasecmp(const char *a, const char *b)
{
    for (; *a && tolower((unsigned char)*a) == tolower((unsigned char)*b); a++, b++) ;
    return tolower((unsigned char)*a) - tolower((unsigned char)*b);
}
int strncasecmp(const char *a, const char *b, size_t n)
{
    for (; n && *a && tolower((unsigned char)*a) == tolower((unsigned char)*b); a++, b++, n--) ;
    return n ? tolower((unsigned char)*a) - tolower((unsigned char)*b) : 0;
}
char *strchr(const char *s, int c)
{
    for (;; s++) {
        if (*s == (char)c) return (char *)s;
        if (!*s) return 0;
    }
}
char *strrchr(const char *s, int c)
{
    const char *r = 0;
    for (;; s++) {
        if (*s == (char)c) r = s;
        if (!*s) return (char *)r;
    }
}
char *strstr(const char *h, const char *n)
{
    size_t l = strlen(n);
    if (!l) return (char *)h;
    for (; *h; h++)
        if (*h == *n && !strncmp(h, n, l)) return (char *)h;
    return 0;
}
char *strdup(const char *s)
{
    size_t n = strlen(s) + 1;
    char *d = malloc(n);
    if (d) memcpy(d, s, n);
    return d;
}

int abs(int v) { return v < 0 ? -v : v; }
long labs(long v) { return v < 0 ? -v : v; }
double fabs(double v) { return v < 0 ? -v : v; }

/*
 * Lo justo de math.h para las tablas del emulador OPL (DBOPL_InitTables y
 * Chip__Setup): sólo se usa al arrancar, así que vale la precisión de series.
 */
#define CXD_LN2 0.69314718055994530942
#define CXD_PI 3.14159265358979323846

double log(double x)
{
    union { double d; uint64_t u; } v;
    double m, t, t2, sum = 0, term;
    int e, k;
    if (x <= 0) return -1e308;
    v.d = x;
    e = (int)((v.u >> 52) & 0x7ff) - 1023;
    v.u = (v.u & 0x000fffffffffffffull) | 0x3ff0000000000000ull;   /* m en [1, 2) */
    m = v.d;
    t = (m - 1) / (m + 1);                                       /* ln m = 2 atanh t */
    t2 = t * t;
    term = t;
    for (k = 1; k < 40; k += 2) { sum += term / k; term *= t2; }
    return 2 * sum + e * CXD_LN2;
}

double log10(double x) { return log(x) / 2.30258509299404568402; }

double exp(double x)
{
    union { double d; uint64_t u; } v;
    double r, sum = 1, term = 1;
    int k, n;
    if (x > 709) return 1e308;
    if (x < -708) return 0;
    k = (int)(x / CXD_LN2 + (x < 0 ? -0.5 : 0.5));
    r = x - k * CXD_LN2;
    for (n = 1; n < 25; n++) { term *= r / n; sum += term; }
    v.d = sum;
    v.u += (uint64_t)(int64_t)k << 52;
    return v.d;
}

double pow(double x, double y)
{
    if (y == 0) return 1;
    if (x <= 0) return 0;
    return exp(y * log(x));
}

double sin(double x)
{
    double x2, term, sum;
    int n, neg = 0;
    while (x > CXD_PI) x -= 2 * CXD_PI;
    while (x < -CXD_PI) x += 2 * CXD_PI;
    if (x < 0) { x = -x; neg = 1; }
    if (x > CXD_PI / 2) x = CXD_PI - x;
    x2 = x * x;
    term = x;
    sum = x;
    for (n = 1; n < 12; n++) { term *= -x2 / ((2 * n) * (2 * n + 1)); sum += term; }
    return neg ? -sum : sum;
}

double cos(double x) { return sin(x + CXD_PI / 2); }
double floor(double x) { double t = (double)(long long)x; return t > x ? t - 1 : t; }
double ceil(double x) { double t = (double)(long long)x; return t < x ? t + 1 : t; }

long strtol(const char *s, char **end, int base)
{
    long v = 0;
    int neg = 0, d;
    while (isspace((unsigned char)*s)) s++;
    if (*s == '-') { neg = 1; s++; } else if (*s == '+') s++;
    if ((base == 0 || base == 16) && s[0] == '0' && (s[1] == 'x' || s[1] == 'X')) { s += 2; base = 16; }
    else if (base == 0 && s[0] == '0') base = 8;
    else if (base == 0) base = 10;
    for (;; s++) {
        if (isdigit((unsigned char)*s)) d = *s - '0';
        else if (isalpha((unsigned char)*s)) d = tolower((unsigned char)*s) - 'a' + 10;
        else break;
        if (d >= base) break;
        v = v * base + d;
    }
    if (end) *end = (char *)s;
    return neg ? -v : v;
}
int atoi(const char *s) { return (int)strtol(s, 0, 10); }
double atof(const char *s)
{
    double v = 0, scale = 1;
    int neg = 0;
    while (isspace((unsigned char)*s)) s++;
    if (*s == '-') { neg = 1; s++; }
    for (; isdigit((unsigned char)*s); s++) v = v * 10 + (*s - '0');
    if (*s == '.')
        for (s++; isdigit((unsigned char)*s); s++) { scale /= 10; v += (*s - '0') * scale; }
    return neg ? -v : v;
}

/* --- montón ----------------------------------------------------------- */

/* Lista de bloques libres ordenada por dirección; el tamaño incluye la cabecera. */
struct blk { size_t size; struct blk *next; };
static struct blk *freelist;

void cxd_heap_init(void *base, size_t size)
{
    freelist = base;
    freelist->size = size & ~(size_t)7;
    freelist->next = 0;
}

void *malloc(size_t n)
{
    struct blk **pp, *b;
    n = (n + sizeof(struct blk) + 7) & ~(size_t)7;
    for (pp = &freelist; (b = *pp); pp = &b->next) {
        if (b->size < n) continue;
        if (b->size - n >= 2 * sizeof(struct blk)) {
            struct blk *rest = (struct blk *)((char *)b + n);
            rest->size = b->size - n;
            rest->next = b->next;
            b->size = n;
            *pp = rest;
        } else {
            *pp = b->next;
        }
        return b + 1;
    }
    errno = ENOMEM;
    return 0;
}

void free(void *p)
{
    struct blk *b, *prev = 0, *cur;
    if (!p) return;
    b = (struct blk *)p - 1;
    for (cur = freelist; cur && cur < b; cur = cur->next) prev = cur;
    b->next = cur;
    if (prev) prev->next = b; else freelist = b;
    if (cur && (char *)b + b->size == (char *)cur) { b->size += cur->size; b->next = cur->next; }
    if (prev && (char *)prev + prev->size == (char *)b) { prev->size += b->size; prev->next = b->next; }
}

void *calloc(size_t a, size_t b)
{
    void *p = malloc(a * b);
    if (p) memset(p, 0, a * b);
    return p;
}

/*
 * Al crecer se pide el doble: el lector de MIDI agranda su lista de eventos de
 * a uno, y copiar todo en cada evento tardaba 37 s en la cámara con E1M1 (v42).
 */
void *realloc(void *p, size_t n)
{
    size_t old;
    void *q;
    if (!p) return malloc(n);
    old = ((struct blk *)p - 1)->size - sizeof(struct blk);
    if (n <= old) return p;
    q = malloc(n < old * 2 ? old * 2 : n);
    if (!q) q = malloc(n);
    if (!q) return 0;
    memcpy(q, p, old);
    free(p);
    return q;
}

/* --- printf ----------------------------------------------------------- */

int vsnprintf(char *buf, size_t cap, const char *fmt, va_list ap)
{
    size_t n = 0;
#define PUT(c) do { if (n + 1 < cap) buf[n] = (char)(c); n++; } while (0)
    for (; *fmt; fmt++) {
        char tmp[40], sign = 0, pad;
        const char *s = tmp;
        int left = 0, zero = 0, plus = 0, width = 0, prec = -1, lng = 0, len = 0, i;
        unsigned base = 10;
        unsigned long long u = 0;
        int numeric = 0, upper = 0;
        if (*fmt != '%') { PUT(*fmt); continue; }
        for (fmt++;; fmt++) {
            if (*fmt == '-') left = 1;
            else if (*fmt == '0') zero = 1;
            else if (*fmt == '+') plus = 1;
            else if (*fmt != ' ' && *fmt != '#') break;
        }
        if (*fmt == '*') { width = va_arg(ap, int); if (width < 0) { left = 1; width = -width; } fmt++; }
        else for (; isdigit((unsigned char)*fmt); fmt++) width = width * 10 + (*fmt - '0');
        if (*fmt == '.') {
            prec = 0;
            fmt++;
            if (*fmt == '*') { prec = va_arg(ap, int); fmt++; }
            else for (; isdigit((unsigned char)*fmt); fmt++) prec = prec * 10 + (*fmt - '0');
        }
        for (; *fmt == 'l' || *fmt == 'h' || *fmt == 'z'; fmt++)
            if (*fmt == 'l') lng++;
        switch (*fmt) {
        case 'd': case 'i': {
            long long v = lng >= 2 ? va_arg(ap, long long) : (long long)va_arg(ap, int);
            if (v < 0) { sign = '-'; u = (unsigned long long)-v; }
            else { u = (unsigned long long)v; if (plus) sign = '+'; }
            numeric = 1;
            break;
        }
        case 'X': upper = 1; /* sigue */
        case 'x': base = 16; /* sigue */
        case 'u':
            u = lng >= 2 ? va_arg(ap, unsigned long long) : (unsigned long long)va_arg(ap, unsigned);
            numeric = 1;
            break;
        case 'o':
            base = 8;
            u = va_arg(ap, unsigned);
            numeric = 1;
            break;
        case 'p':
            base = 16;
            u = (uintptr_t)va_arg(ap, void *);
            numeric = 1;
            break;
        case 'c':
            tmp[0] = (char)va_arg(ap, int);
            len = 1;
            break;
        case 's':
            s = va_arg(ap, const char *);
            if (!s) s = "(null)";
            for (len = 0; s[len] && (prec < 0 || len < prec); len++) ;
            break;
        case 'f': case 'g': case 'e': {
            /* Parte entera, punto y `prec` decimales (6 si no se dice). */
            double v = va_arg(ap, double);
            unsigned long ip;
            int k, pr = prec < 0 ? 6 : prec > 9 ? 9 : prec;
            char *t = tmp + sizeof(tmp);
            if (v < 0) { sign = '-'; v = -v; }
            ip = (unsigned long)v;
            v -= (double)ip;
            for (k = 0; k < pr; k++) v *= 10;
            {
                unsigned long fp = (unsigned long)(v + 0.5);
                for (k = 0; k < pr; k++) { *--t = (char)('0' + fp % 10); fp /= 10; }
                if (pr) *--t = '.';
            }
            do { *--t = (char)('0' + ip % 10); ip /= 10; } while (ip);
            s = t;
            len = (int)(tmp + sizeof(tmp) - t);
            prec = -1;
            break;
        }
        case '%':
            tmp[0] = '%';
            len = 1;
            break;
        case 0:
            fmt--;
            continue;
        default:
            tmp[0] = '%';
            tmp[1] = *fmt;
            len = 2;
            break;
        }
        if (numeric) {
            char *t = tmp + sizeof(tmp);
            const char *digits = upper ? "0123456789ABCDEF" : "0123456789abcdef";
            do { *--t = digits[u % base]; u /= base; } while (u);
            len = (int)(tmp + sizeof(tmp) - t);
            while (len < prec && t > tmp) { *--t = '0'; len++; }
            s = t;
        }
        pad = (zero && !left && numeric && prec < 0) ? '0' : ' ';
        i = width - len - (sign ? 1 : 0);
        if (pad == '0' && sign) { PUT(sign); sign = 0; }
        if (!left) for (; i > 0; i--) PUT(pad);
        if (sign) PUT(sign);
        while (len--) PUT(*s++);
        if (left) for (; i > 0; i--) PUT(' ');
    }
    if (cap) buf[n < cap ? n : cap - 1] = 0;
#undef PUT
    return (int)n;
}

int snprintf(char *buf, size_t cap, const char *fmt, ...)
{
    va_list ap;
    int n;
    va_start(ap, fmt);
    n = vsnprintf(buf, cap, fmt, ap);
    va_end(ap);
    return n;
}

int sprintf(char *buf, const char *fmt, ...)
{
    va_list ap;
    int n;
    va_start(ap, fmt);
    n = vsnprintf(buf, 0x7fffffff, fmt, ap);
    va_end(ap);
    return n;
}

/* --- salida de texto: por líneas a la bitácora ------------------------ */

static char line[150];
static unsigned line_n;

void cxd_out(const char *s, size_t n)
{
    for (; n; n--, s++) {
        if (*s == '\n' || line_n == sizeof(line) - 1) {
            line[line_n] = 0;
            if (line_n) cxd_log_line(line);
            line_n = 0;
            if (*s == '\n') continue;
        }
        if (*s != '\r') line[line_n++] = *s;
    }
}

/* mem: archivo en memoria (de la tabla mfile), con su posición; writing: se está escribiendo. */
struct cxd_file { int fd, eof, console, mem, writing; unsigned pos; };
static struct cxd_file f_out = { -1, 0, 1, -1, 0, 0 }, f_err = { -1, 0, 1, -1, 0, 0 }, f_in = { -1, 1, 0, -1, 0, 0 };
FILE *stdout = &f_out, *stderr = &f_err, *stdin = &f_in;
#define NFILE 6
static struct cxd_file files[NFILE];

#define NMFILE 4
static struct { char name[64]; unsigned char *data; unsigned size, cap; int used; } mfile[NMFILE];

static int mfind(const char *name)
{
    int i;
    for (i = 0; i < NMFILE; i++)
        if (mfile[i].used && !strcmp(mfile[i].name, name)) return i;
    return -1;
}

static void mdrop(int i)
{
    free(mfile[i].data);
    mfile[i].data = 0;
    mfile[i].used = 0;
}

int vfprintf(FILE *f, const char *fmt, va_list ap)
{
    char b[400];
    int n = vsnprintf(b, sizeof(b), fmt, ap);
    if (f && f->console) cxd_out(b, strlen(b));
    return n;
}

int fprintf(FILE *f, const char *fmt, ...)
{
    va_list ap;
    int n;
    va_start(ap, fmt);
    n = vfprintf(f, fmt, ap);
    va_end(ap);
    return n;
}

int printf(const char *fmt, ...)
{
    va_list ap;
    int n;
    va_start(ap, fmt);
    n = vfprintf(stdout, fmt, ap);
    va_end(ap);
    return n;
}

int puts(const char *s) { cxd_out(s, strlen(s)); cxd_out("\n", 1); return 0; }
int putchar(int c) { char ch = (char)c; cxd_out(&ch, 1); return c; }

/* --- archivos: sólo lectura ------------------------------------------- */

static FILE *slot_file(void)
{
    int i;
    for (i = 0; i < NFILE; i++)
        if (files[i].fd < 0 && files[i].mem < 0) {
            files[i].eof = files[i].console = files[i].writing = 0;
            files[i].pos = 0;
            return &files[i];
        }
    errno = ENOMEM;
    return 0;
}

FILE *fopen(const char *path, const char *mode)
{
    FILE *f;
    long fd;
    int m;
    if (!path || !mode) { errno = EINVAL; return 0; }
    if (!files[0].console && files[0].fd == 0 && files[0].mem == 0) {   /* primera vez: todo libre */
        int i;
        for (i = 0; i < NFILE; i++) { files[i].fd = -1; files[i].mem = -1; }
    }
    if (mode[0] == 'w') {              /* un archivo en memoria, nuevo */
        if (strlen(path) >= sizeof(mfile[0].name)) { errno = EINVAL; return 0; }
        m = mfind(path);
        if (m >= 0) mdrop(m);
        for (m = 0; m < NMFILE && mfile[m].used; m++) ;
        if (m == NMFILE) { errno = ENOMEM; return 0; }
        if (!(f = slot_file())) return 0;
        strcpy(mfile[m].name, path);
        mfile[m].used = 1;
        mfile[m].size = mfile[m].cap = 0;
        mfile[m].data = 0;
        f->mem = m;
        f->writing = 1;
        return f;
    }
    if (mode[0] != 'r' || strchr(mode, '+')) { errno = EACCES; return 0; }
    m = mfind(path);
    if (m >= 0) {
        if (!(f = slot_file())) return 0;
        f->mem = m;
        return f;
    }
    if (!(f = slot_file())) return 0;
    fd = cxd_sys(SYS_open, (long)path, O_RDONLY, 0, 0, 0, 0);
    if (fd < 0) { errno = (int)-fd; return 0; }
    f->fd = (int)fd;
    return f;
}

int fclose(FILE *f)
{
    if (!f || f->console) return 0;
    if (f->fd >= 0) cxd_sys(SYS_close, f->fd, 0, 0, 0, 0, 0);
    f->fd = -1;
    f->mem = -1;
    return 0;
}

size_t fread(void *ptr, size_t size, size_t count, FILE *f)
{
    size_t want = size * count, got = 0;
    if (!f || !want) return 0;
    if (f->mem >= 0) {
        unsigned avail = mfile[f->mem].size - f->pos;
        got = want < avail ? want : avail;
        memcpy(ptr, mfile[f->mem].data + f->pos, got);
        f->pos += got;
        if (got < want) f->eof = 1;
        return got / size;
    }
    if (f->fd < 0) return 0;
    while (got < want) {
        long r = cxd_sys(SYS_read, f->fd, (long)((char *)ptr + got), (long)(want - got), 0, 0, 0);
        if (r <= 0) { f->eof = 1; break; }
        got += (size_t)r;
    }
    return got == want ? count : got / size;
}

size_t fwrite(const void *ptr, size_t size, size_t count, FILE *f)
{
    size_t n = size * count;
    if (!f) return 0;
    if (f->console) { cxd_out(ptr, n); return count; }
    if (f->mem >= 0 && f->writing) {
        int m = f->mem;
        if (mfile[m].size + n > mfile[m].cap) {
            unsigned cap = (mfile[m].size + n) * 2 + 256;
            unsigned char *d = realloc(mfile[m].data, cap);
            if (!d) return 0;
            mfile[m].data = d;
            mfile[m].cap = cap;
        }
        memcpy(mfile[m].data + mfile[m].size, ptr, n);
        mfile[m].size += n;
        return count;
    }
    return 0;
}

int fseek(FILE *f, long off, int whence)
{
    if (!f) return -1;
    f->eof = 0;
    if (f->mem >= 0) {
        long base = whence == SEEK_SET ? 0 : whence == SEEK_CUR ? (long)f->pos : (long)mfile[f->mem].size;
        if (base + off < 0 || base + off > (long)mfile[f->mem].size) return -1;
        f->pos = (unsigned)(base + off);
        return 0;
    }
    if (f->fd < 0) return -1;
    return cxd_sys(SYS_lseek, f->fd, off, whence, 0, 0, 0) < 0 ? -1 : 0;
}

long ftell(FILE *f)
{
    if (!f) return -1;
    if (f->mem >= 0) return (long)f->pos;
    if (f->fd < 0) return -1;
    return cxd_sys(SYS_lseek, f->fd, 0, SEEK_CUR, 0, 0, 0);
}

int fgetc(FILE *f)
{
    unsigned char c;
    return fread(&c, 1, 1, f) == 1 ? c : EOF;
}

int fflush(FILE *f) { (void)f; return 0; }
int feof(FILE *f) { return f ? f->eof : 1; }

/* Lo justo para M_StrToInt: espacios, texto literal y %d %i %x %o %u. */
int sscanf(const char *s, const char *fmt, ...)
{
    va_list ap;
    int n = 0;
    va_start(ap, fmt);
    for (; *fmt; fmt++) {
        if (isspace((unsigned char)*fmt)) { while (isspace((unsigned char)*s)) s++; continue; }
        if (*fmt != '%') { if (*s != *fmt) break; s++; continue; }
        fmt++;
        {
            int base = *fmt == 'x' ? 16 : *fmt == 'o' ? 8 : *fmt == 'i' ? 0 : 10;
            char *end;
            long v;
            if (!strchr("dixou", *fmt) || !*fmt) break;
            v = strtol(s, &end, base);
            if (end == s) break;
            *va_arg(ap, int *) = (int)v;
            s = end;
            n++;
        }
    }
    va_end(ap);
    return n;
}

/* --- lo que no existe aquí -------------------------------------------- */

int system(const char *cmd) { (void)cmd; return -1; }
int mkdir(const char *path, mode_t mode) { (void)path; (void)mode; return -1; }
int remove(const char *path)
{
    int m = path ? mfind(path) : -1;
    if (m < 0) return -1;
    mdrop(m);
    return 0;
}
int rename(const char *a, const char *b) { (void)a; (void)b; return -1; }
char *getenv(const char *name) { (void)name; return 0; }
int raise(int sig) { (void)sig; return 0; }
void exit(int code) { cxd_exit(code); }
void cxd_assert_fail(const char *what, const char *file, int line)
{
    char b[160];
    snprintf(b, sizeof(b), "assert: %s (%s:%d)", what, file, line);
    cxd_log_line(b);
    cxd_exit(-3);
}
void abort(void) { cxd_exit(-2); }
void __aeabi_unwind_cpp_pr0(void) {}
void __aeabi_unwind_cpp_pr1(void) {}
void __aeabi_unwind_cpp_pr2(void) {}
