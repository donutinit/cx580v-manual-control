/*
 * cxinh: copia la tabla de bloqueos de funciones (MWF::InhInfo) a /setting.
 *
 * Sólo lee. Corre en un hilo propio de im.elf, creado con clone() y sin libc,
 * para no tocar ninguna función de la app. A los CX_T1 y CX_T2 segundos de
 * arrancar copia las entradas distintas de cero de la tabla a CX_INH_PATH. Si
 * el archivo ya existe no hace nada, así que hay una sola copia por instalación.
 *
 * La tabla: MWF::InhInfo::mp_inhTbl apunta a FEAT_MAX x WORDS palabras de 32
 * bits; el bit b de la palabra w de la función f indica que el factor
 * w*32 + b la bloquea (GetInhTblValue, libMWF.so 1.07 en 0x1d1d8).
 *
 * Antes de leer se comprueba que libMWF.so sea la de la versión 1.07 y que la
 * tabla esté dentro de memoria mapeada. Si algo no cuadra, no se lee nada.
 */

#include "cx.h"

#ifndef CX_INH_PATH
#define CX_INH_PATH "/setting/cxinh.bin"
#endif
#ifndef CX_T1
#define CX_T1 20
#endif
#ifndef CX_T2
#define CX_T2 90
#endif

/* libMWF.so de la versión 1.07 (sha256 en docs/research/firmware-analysis.md). */
#define MWF_NAME          "/libMWF.so"
#define MWF_MP_INHTBL     0x45e5c   /* _ZN3MWF7InhInfo9mp_inhTblE */
#define MWF_GETINHTBL     0x1d1d8   /* _ZN3MWF7InhInfo14GetInhTblValueEjj */
#define MWF_GETINHINFO    0x1d3a4   /* _ZN3MWF7InhInfo10GetInhInfoE... */
#define MWF_SIG_PUSH      0xe92d4070 /* push {r4, r5, r6, lr} */
#define MWF_SIG_GETTBL2   0xe1a05000 /* mov r5, r0 */
#define FEAT_MAX          0x403     /* DefInh::GetFeatureIdMax() */
#define WORDS             58        /* DefInh::GetFactorTblMax() */

#define SNAP_MAX 8192    /* dos copias caben en un cluster de 16 KiB */

struct snap_head {
    char magic[4];       /* "CXI1" */
    u32 seconds;
    u32 feat_max;
    u32 words;
    u32 mwf_base;
    u32 tbl;
    u32 status;          /* 0 ok, otro valor: por qué no se leyó */
    u32 nonzero_total;
    u32 nrec;
};

struct rec { u16 feature; u16 word; u32 value; };

static u8 snap[SNAP_MAX] __attribute__((aligned(8)));

/* --- /proc/self/maps -------------------------------------------------- */

struct map { u32 start, end; char perm[5]; u32 offset; int is_mwf; };

static int hexval(char c)
{
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    return -1;
}

/* "start-end perms offset ..." -> map. Returns 0 on success. */
static int parse_line(const char *l, int n, struct map *m)
{
    int i = 0, v, k;
    u32 x;
    for (k = 0; k < 2; k++) {
        x = 0;
        while (i < n && (v = hexval(l[i])) >= 0) { x = (x << 4) | v; i++; }
        if (k == 0) { m->start = x; if (i >= n || l[i] != '-') return -1; }
        else m->end = x;
        i++;
    }
    for (k = 0; k < 4 && i < n; k++) m->perm[k] = l[i++];
    m->perm[4] = 0;
    i++;
    x = 0;
    while (i < n && (v = hexval(l[i])) >= 0) { x = (x << 4) | v; i++; }
    m->offset = x;
    /* ¿termina en /libMWF.so? */
    m->is_mwf = 0;
    {
        const char *s = MWF_NAME;
        int sl = 0, j;
        while (s[sl]) sl++;
        if (n >= sl) {
            for (j = 0; j < sl && l[n - sl + j] == s[j]; j++) ;
            m->is_mwf = (j == sl);
        }
    }
    return 0;
}

/*
 * Walks /proc/self/maps. Finds the base of libMWF.so (r-xp, offset 0), and
 * checks whether [lo, hi) lies inside one readable mapping.
 */
static int scan_maps(u32 *mwf_base, u32 lo, u32 hi, int *contained)
{
    static char chunk[1024];
    char line[256];
    int ll = 0;
    long fd = cx_open("/proc/self/maps", O_RDONLY, 0), n, i;
    struct map m;
    m.start = m.end = m.offset = 0;
    m.is_mwf = 0;
    if (fd < 0) return -1;
    *mwf_base = 0;
    *contained = 0;
    while ((n = cx_read(fd, chunk, sizeof(chunk))) > 0) {
        for (i = 0; i < n; i++) {
            char c = chunk[i];
            if (c != '\n') {
                if (ll < (int)sizeof(line)) line[ll++] = c;
                continue;
            }
            if (parse_line(line, ll, &m) == 0) {
                if (m.is_mwf && m.offset == 0 && m.perm[0] == 'r' && m.perm[2] == 'x')
                    *mwf_base = m.start;
                if (m.perm[0] == 'r' && lo >= m.start && hi <= m.end && lo < hi)
                    *contained = 1;
            }
            ll = 0;
        }
    }
    cx_close(fd);
    return 0;
}

/* --- snapshot -------------------------------------------------------- */

static u32 take_snapshot(u32 seconds)
{
    struct snap_head *h = (struct snap_head *)snap;
    struct rec *r = (struct rec *)(snap + sizeof(*h));
    u32 maxrec = (SNAP_MAX - sizeof(*h)) / sizeof(*r);
    u32 base = 0, tbl, f, w, v;
    int ok = 0;
    const volatile u32 *t;

    h->magic[0] = 'C'; h->magic[1] = 'X'; h->magic[2] = 'I'; h->magic[3] = '1';
    h->seconds = seconds;
    h->feat_max = FEAT_MAX;
    h->words = WORDS;
    h->mwf_base = 0;
    h->tbl = 0;
    h->nonzero_total = 0;
    h->nrec = 0;

    if (scan_maps(&base, 0, 0, &ok) != 0) { h->status = 1; goto out; }
    h->mwf_base = base;
    if (!base) { h->status = 2; goto out; }
    if (*(volatile u32 *)(base + MWF_GETINHTBL) != MWF_SIG_PUSH ||
        *(volatile u32 *)(base + MWF_GETINHTBL + 4) != MWF_SIG_GETTBL2 ||
        *(volatile u32 *)(base + MWF_GETINHINFO) != MWF_SIG_PUSH) { h->status = 3; goto out; }
    if (scan_maps(&base, base + MWF_MP_INHTBL, base + MWF_MP_INHTBL + 4, &ok) != 0 || !ok) { h->status = 4; goto out; }
    tbl = *(volatile u32 *)(base + MWF_MP_INHTBL);
    h->tbl = tbl;
    if (!tbl) { h->status = 5; goto out; }
    if (tbl & 3) { h->status = 6; goto out; }
    if (scan_maps(&base, tbl, tbl + FEAT_MAX * WORDS * 4, &ok) != 0 || !ok) { h->status = 7; goto out; }

    t = (const volatile u32 *)tbl;
    for (f = 0; f < FEAT_MAX; f++)
        for (w = 0; w < WORDS; w++) {
            v = t[f * WORDS + w];
            if (!v) continue;
            h->nonzero_total++;
            if (h->nrec < maxrec) {
                r[h->nrec].feature = f;
                r[h->nrec].word = w;
                r[h->nrec].value = v;
                h->nrec++;
            }
        }
    h->status = 0;
out:
    return sizeof(*h) + h->nrec * sizeof(*r);
}

static int write_snapshot(u32 seconds, int first)
{
    u32 len = take_snapshot(seconds);
    long fd = first ? cx_open(CX_INH_PATH, O_WRONLY | O_CREAT | O_EXCL, 0666)
                    : cx_open(CX_INH_PATH, O_WRONLY | O_APPEND, 0);
    if (fd < 0) return -1;
    cx_write(fd, snap, len);
    cx_fsync(fd);
    cx_close(fd);
    return 0;
}

static void cxinh_thread(void)
{
    u32 all[2] = { 0xffffffff, 0xffffffff };
    /* Que ninguna señal del proceso se entregue en este hilo. */
    sys6(SYS_rt_sigprocmask, 0 /* SIG_BLOCK */, (long)all, 0, 8 /* sigsetsize */, 0, 0);

    cx_sleep(CX_T1);
    if (write_snapshot(CX_T1, 1) != 0) return;
    cx_sleep(CX_T2 - CX_T1);
    write_snapshot(CX_T2, 0);
}

void cxinh_start(void)
{
    long fd = cx_open(CX_INH_PATH, O_RDONLY, 0);
    if (fd >= 0) { cx_close(fd); return; }   /* ya hay una copia */
    cx_thread(cxinh_thread);
}

/* --- hilo sin libc ------------------------------------------------- */

#define CLONE_FLAGS (0x100 | 0x200 | 0x400 | 0x800 | 0x10000 | 0x40000)
/* VM | FS | FILES | SIGHAND | THREAD | SYSVSEM */
#define STACK_SIZE 65536

long cx_clone(long flags, void *stack, void (*fn)(void));
__asm__(
    ".text\n"
    ".arm\n"
    ".align 2\n"
    ".type cx_clone, %function\n"
    "cx_clone:\n"
    "    push {r4, r5, r7, lr}\n"
    "    mov r5, r2\n"          /* fn: r5 se copia al hilo nuevo */
    "    mov r2, #0\n"          /* parent_tidptr */
    "    mov r3, #0\n"          /* tls */
    "    mov r4, #0\n"          /* child_tidptr */
    "    mov r7, #120\n"        /* clone */
    "    svc 0\n"
    "    cmp r0, #0\n"
    "    beq 1f\n"
    "    pop {r4, r5, r7, pc}\n"
    "1:  blx r5\n"
    "    mov r0, #0\n"
    "    mov r7, #1\n"          /* exit: termina sólo este hilo */
    "    svc 0\n"
    "2:  b 2b\n"
    ".size cx_clone, .-cx_clone\n"
);

int cx_thread(void (*fn)(void))
{
    long stack = sys6(SYS_mmap2, 0, STACK_SIZE, 3 /* RW */, 0x22 /* PRIVATE|ANON */, -1, 0);
    if (stack < 0 && stack > -4096) return -1;
    return cx_clone(CLONE_FLAGS, (void *)(stack + STACK_SIZE), fn) < 0 ? -1 : 0;
}
