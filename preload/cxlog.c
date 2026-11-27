/*
 * cxlog: bitácora en texto hacia un archivo de /setting, sin libc.
 *
 * Los hilos de la app escriben en un búfer en memoria (sin llamadas al kernel
 * mientras tienen el candado). Un hilo propio lo vuelca al archivo cada
 * CX_FLUSH_SECS segundos, hasta CX_LOG_MAX bytes. Si el archivo ya existe al
 * arrancar, la bitácora queda apagada: una sola por instalación.
 * Todo en hexadecimal, para no necesitar divisiones.
 */

#include "cx.h"

#ifndef CX_LOG_PATH
#define CX_LOG_PATH "/setting/cxreq.log"
#endif
#ifndef CX_LOG_MAX
#define CX_LOG_MAX (48 * 1024)
#endif
#ifndef CX_FLUSH_SECS
#define CX_FLUSH_SECS 5
#endif
#ifndef CX_LOG_SECS
#define CX_LOG_SECS 900      /* deja de registrar a los 15 min */
#endif

#define BUF 16384

static char buf[BUF];
static u32 used;
static u32 dropped;
static volatile int lk;
static int enabled;
static u32 t0;

static void lock(void)
{
    while (__sync_lock_test_and_set(&lk, 1))
        sys3(158 /* sched_yield */, 0, 0, 0);
}

static void unlock(void) { __sync_lock_release(&lk); }

u32 cx_now(u32 *frac)
{
    long ts[2] = { 0, 0 };
    sys3(263 /* clock_gettime */, 1 /* CLOCK_MONOTONIC */, ts, 0);
    if (frac) *frac = (u32)ts[1] >> 20;   /* ~1 ms */
    return (u32)ts[0];
}

/* --- formato --------------------------------------------------------- */

void cxl_str(struct line *l, const char *s, int max)
{
    int i;
    for (i = 0; s && s[i] && i < max && l->n < (int)sizeof(l->b) - 2; i++) {
        char c = s[i];
        l->b[l->n++] = (c >= 32 && c < 127 && c != '"') ? c : '?';
    }
}

/* Texto fijo del propio cxg.so, sin sanear. */
void cxl_lit(struct line *l, const char *s)
{
    while (*s && l->n < (int)sizeof(l->b) - 2) l->b[l->n++] = *s++;
}

void cxl_hex(struct line *l, u32 v)
{
    static const char hx[] = "0123456789abcdef";
    int i, started = 0;
    for (i = 28; i >= 0; i -= 4) {
        int d = (v >> i) & 15;
        if (d || started || i == 0) {
            if (l->n < (int)sizeof(l->b) - 2) l->b[l->n++] = hx[d];
            started = 1;
        }
    }
}

void cxl_begin(struct line *l, const char *kind)
{
    u32 frac, s = cx_now(&frac) - t0;
    l->n = 0;
    cxl_hex(l, s);
    cxl_str(l, ".", 1);
    cxl_hex(l, frac);
    cxl_str(l, " ", 1);
    cxl_str(l, kind, 8);
}

void cxl_end(struct line *l)
{
    l->b[l->n++] = '\n';
    if (!enabled) return;
    if (cx_now(0) - t0 > CX_LOG_SECS) { enabled = 0; return; }
    lock();
    if (used + l->n <= BUF) {
        int i;
        for (i = 0; i < l->n; i++) buf[used + i] = l->b[i];
        used += l->n;
    } else {
        dropped++;
    }
    unlock();
}

int cx_log_enabled(void) { return enabled; }

/* --- volcado ----------------------------------------------------------- */

static volatile int want_sync;
static volatile u32 sync_gen;

/*
 * Espera (hasta 1.5 s) a que lo anotado hasta ahora esté en el archivo. Para
 * los pasos que pueden tumbar la aplicación: si se cae, la bitácora dice dónde.
 */
void cx_log_sync(void)
{
    u32 gen = sync_gen;
    int i;
    if (!enabled) return;
    want_sync = 1;
    for (i = 0; i < 60 && sync_gen == gen && enabled; i++) cx_sleep_ms(25);
}

static char out[BUF];

static void flusher(void)
{
    u32 all[2] = { 0xffffffff, 0xffffffff };
    u32 total = 0, n, last_dropped = 0;
    long fd;
    int first = 1;

    sys6(SYS_rt_sigprocmask, 0, (long)all, 0, 8, 0, 0);
    while (total < CX_LOG_MAX) {
        {
            int k;   /* en rebanadas, para atender cx_log_sync() sin esperar el plazo */
            for (k = 0; k < CX_FLUSH_SECS * 20 && !want_sync; k++) cx_sleep_ms(50);
        }
        lock();
        n = used;
        if (n > CX_LOG_MAX - total) n = CX_LOG_MAX - total;
        {
            u32 i;
            for (i = 0; i < n; i++) out[i] = buf[i];
            for (i = n; i < used; i++) buf[i - n] = buf[i];
            used -= n;
        }
        unlock();
        if (dropped != last_dropped) {
            struct line l;
            cxl_begin(&l, "DROP");
            cxl_str(&l, " ", 1);
            cxl_hex(&l, dropped);
            l.b[l.n++] = '\n';
            if (n + l.n <= BUF) {
                int i;
                for (i = 0; i < l.n; i++) out[n + i] = l.b[i];
                n += l.n;
            }
            last_dropped = dropped;
        }
        if (!n) {
            want_sync = 0;
            sync_gen++;
            if (!enabled) break;
            continue;
        }
        fd = first ? cx_open(CX_LOG_PATH, O_WRONLY | O_CREAT | O_EXCL, 0666)
                   : cx_open(CX_LOG_PATH, O_WRONLY | O_APPEND, 0);
        if (fd < 0) { enabled = 0; break; }
        first = 0;
        cx_write(fd, out, n);
        cx_fsync(fd);
        cx_close(fd);
        total += n;
        want_sync = 0;
        sync_gen++;
    }
    enabled = 0;
}

void cx_log_start(void)
{
    long fd = cx_open(CX_LOG_PATH, O_RDONLY, 0);
    t0 = cx_now(0);
    if (fd >= 0) { cx_close(fd); return; }   /* ya hay una bitácora */
    enabled = 1;
    if (cx_thread(flusher) != 0) enabled = 0;
}
