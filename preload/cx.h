/* Llamadas al kernel (ARM EABI) sin libc, compartidas por los módulos de cxg.so. */
#pragma once

#define SYS_exit           1
#define SYS_read           3
#define SYS_write          4
#define SYS_open           5
#define SYS_close          6
#define SYS_fsync          118
#define SYS_clone          120
#define SYS_nanosleep      162
#define SYS_rt_sigprocmask 175
#define SYS_mmap2          192

#define O_RDONLY  0
#define O_WRONLY  1
#define O_CREAT   0100
#define O_EXCL    0200
#define O_APPEND  02000

typedef unsigned int u32;
typedef unsigned short u16;
typedef unsigned char u8;

static inline long sys6(long n, long a, long b, long c, long d, long e, long f)
{
    register long r0 __asm__("r0") = a;
    register long r1 __asm__("r1") = b;
    register long r2 __asm__("r2") = c;
    register long r3 __asm__("r3") = d;
    register long r4 __asm__("r4") = e;
    register long r5 __asm__("r5") = f;
    register long r7 __asm__("r7") = n;
    __asm__ volatile("svc 0" : "+r"(r0) : "r"(r1), "r"(r2), "r"(r3), "r"(r4), "r"(r5), "r"(r7) : "memory");
    return r0;
}

#define sys3(n, a, b, c) sys6((n), (long)(a), (long)(b), (long)(c), 0, 0, 0)

static inline long cx_open(const char *p, int flags, int mode) { return sys3(SYS_open, p, flags, mode); }
static inline long cx_read(long fd, void *b, long n) { return sys3(SYS_read, fd, b, n); }
static inline long cx_write(long fd, const void *b, long n) { return sys3(SYS_write, fd, b, n); }
static inline void cx_close(long fd) { sys3(SYS_close, fd, 0, 0); }
static inline void cx_fsync(long fd) { sys3(SYS_fsync, fd, 0, 0); }

static inline void cx_sleep_ms(long ms)
{
    long ts[2];
    ts[0] = 0; ts[1] = ms * 1000000;   /* ms < 1000; sin división (no hay libgcc) */
    sys3(SYS_nanosleep, (long)ts, 0, 0);
}
static inline void cx_sleep(long seconds)
{
    long ts[2] = { seconds, 0 };
    sys3(SYS_nanosleep, ts, 0, 0);
}

/* Starts fn() on a new thread with its own stack. Used only by im.elf. */
int cx_thread(void (*fn)(void));

/* cxdl.c */
u32 cx_lib_base(const char *suffix);
u32 cx_dlsym(u32 base, const char *name);

/* cxlog.c */
struct line { char b[512]; int n; };
u32 cx_now(u32 *frac);
void cxl_begin(struct line *l, const char *kind);
void cxl_str(struct line *l, const char *s, int max);
void cxl_lit(struct line *l, const char *s);
void cxl_hex(struct line *l, u32 v);
void cxl_end(struct line *l);
int cx_log_enabled(void);
void cx_log_sync(void);
void cx_log_start(void);

/* Module entry points, called from the constructor in cxguard.c. */
void cxinh_start(void);
void cxview_start(void);
void cxobj_start(void);
void cxtext_start(void);
void cxmenu_start(void);
/* cxmenu.c: muestra (show = 1) u oculta el ítem `id` del menú cuya tabla está en `root`. */
void cxmenu_add(u32 root, u32 id, int show);
/* cxtext.c: etiqueta `from` se muestra como `to` (16 bits bajos del número de texto). */
void cxtext_add(u32 from, u32 to);
void cxtext_add_image(u32 from, u32 to);
/* cxtext.c: la etiqueta `from` muestra el texto libre `word` ('_' es espacio). */
void cxtext_add_text(u32 from, const char *word);
#ifdef CX_UX
/* cxux.c: bitácora de tacto y botones (sólo en cxg-ux.so). */
extern int cx_uxlog;
void cxux_start(void);
void cxux_flush(void);
void cxux_note_label(u32 sprite, u32 id);
void cxux_touch(const char *kind, u32 mouse, u32 button, int x, int y);
#endif
/* cxtouch.c: envolturas del tacto. */
void cxtouch_start(void);
/* cxpanel.c: panel propio en la pantalla de grabación. */
extern int cx_panel, cx_panel_draw, cx_panel_dump;
extern int cx_panel_rect[5][4];                     /* x, y, ancho, alto de cada zona */
extern u32 cx_panel_font[5];
void cxpanel_hidebox(int x, int y, int w, int h);   /* oculta lo nativo que quepa en la caja */
void cxpanel_hideid(u32 id);                        /* oculta el widget nativo con ese ID */
void cxpanel_native(u32 widget);                    /* un widget nativo que la interfaz acaba de tocar */
void cxpanel_view(const char *name, int opening);   /* tras la openView/closeView original */
void cxpanel_hook(int h);                           /* desde cualquier envoltura; h: quién llama */
int cxpanel_press(int x, int y);                    /* 1: el toque es del panel */
/* cxobj.c: motor del panel. k: 0 iris, 1 obturación. */
int cxobj_panel_ready(void);
u32 cxobj_panel_get(u32 *fno, u32 *sn, u32 *sd, int *gain_db, int *gain_idx);
void cxobj_panel_manual(int k);
void cxobj_panel_auto(int k);
void cxobj_panel_step(int k, int dir);
void cxobj_panel_gain(int idx);
extern int cx_textlog;

/* Configuración de cxmode.txt que usa cxobj.c (la lee cxview.c). */
extern int cx_objmanual;
/* cxobj.c: 1 desde que se mandó EXP_MODE_MANUAL hasta que pasa otro modo. */
extern volatile int cx_manual_active;
extern int cx_holdmanual;
extern int cx_gainshift;
extern int cx_isoshift;
extern int cx_expogain;
extern int cx_unlockiso;
extern int cx_autounlock;
extern int cx_gainlock;
/* cxobj.c: qué está manual de verdad en la cámara (con holdmanual). */
extern volatile int cx_iris_held, cx_shutter_held;
/* cxobj.c: pone el iris (iris=1) o la obturación en automático o manual en la cámara. */
void cxobj_drive(int iris, int manual);
/* cxobj.c: manda lo pendiente tras un cambio de prioridad (soltar el AE lock y
 * volver a fijar el otro control). Se llama desde el hilo de la interfaz. */
void cxobj_tick(void);
