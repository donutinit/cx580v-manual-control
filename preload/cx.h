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
u32 *cx_got_slot(u32 base, const char *name);      /* entrada de la GOT para una función importada */

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
void cxux_note_event(u32 id);                       /* un evento de EventManager::push (cualquier hilo) */
void cxux_touch(const char *kind, u32 mouse, u32 button, int x, int y);
#endif
/* cxframe.c: cuadros completos en la LCD y la sonda que lo prueba (probe N). */
extern int cx_probe;
void cxframe_start(void);
void cxframe_toggle(void);      /* desde un toque: prueba de memoria y cuadro de prueba */
void cxframe_tick(void);        /* desde el hilo de la interfaz */
int cxframe_open(void);         /* crea el cuadro de 320x200 si falta y lo pone encima; 0 si no se puede */
void cxframe_close(void);
u16 *cxframe_lock(u32 *row, int wait);   /* sus píxeles (`*row` por fila), o 0; wait: 0 no espera, -1 espera */
int cxframe_front(void);        /* lo vuelve a poner encima si algo nuevo lo tapó; 1 si lo movió */
u32 cxframe_state(void);        /* modo del widget | visible << 8 | creado << 9 */
void cxframe_pattern(void);     /* pinta las franjas de la sonda una vez */
void cxframe_clear(void);       /* todo transparente */
int cxframe_prepare(void);      /* creado, encima y transparente; desde un toque u openView/closeView */
int cxframe_ready(void);        /* 1 si ya está creado y visible */
void cxframe_unlock(int changed);
u32 cxframe_free_kb(void);      /* memoria libre más caché, en kB */
/* cxdoom.c: DOOM desde el menú (doom N). */
extern int cx_doom;
extern u32 cx_doom_ms;
void cxdoom_closed(const char *name);               /* después de closeView */
extern u32 cx_doom_mb, cx_doom_hz, cx_doom_vol;
extern int cx_doom_music;
extern u32 cx_doom_mrate, cx_doom_chunk, cx_doom_lead;
extern u8 cx_doom_pix[4];
extern char cx_doom_view[40];
void cxdoom_start(void);
int cxdoom_active(void);
int cxdoom_hides_panel(void);                       /* 1: el juego corre y ya no quiere el panel */
extern u32 cx_doom_panel;
void cxdoom_request(void);                          /* arranca en cuanto se vea la pantalla de grabación */
void cxdoom_touch_start(void);                      /* desde un toque en la pantalla de grabación */
int cxdoom_open(u32 self, const char *name);        /* antes de openView; 1: no se abre */
void cxdoom_touch(int kind, int x, int y);          /* 1 apoyar, 2 arrastrar, 0 soltar */
void cxdoom_tick(void);                             /* desde el hilo de la interfaz */
/* cxview.c: closeView por la envoltura, para que el panel se entere. */
void cxview_close(u32 self, const char *name);
/* cxsound.c: efectos de DOOM por la ruta de los pitidos (doomsound 1). */
extern int cx_sound;
void cxsound_music(const u8 *data, u32 len);        /* un trozo de música, desde el hilo del juego */
u32 cxsound_cap(void);                              /* bytes como máximo por pitido */
void cxsound_start(void);
void cxsound_note(u32 obj, u32 cat, u32 mid);       /* desde IssueCommandAsync: toma el objeto de los pitidos */
void cxsound_play(const u8 *data, u32 len);         /* desde el hilo del juego */
void cxsound_tick(void);                            /* desde el hilo de la interfaz */
void cxsound_stats(void);
/* cxobj.c: manda ObjMsg(0x3410, 0x1120), el pitido `id`, al objeto de los pitidos. */
u32 cxobj_beep(u32 obj, u32 id);
/* cxtouch.c: envolturas del tacto. */
void cxtouch_start(void);
/* cxpanel.c: panel propio en la pantalla de grabación. */
extern int cx_panel, cx_panel_draw, cx_panel_dump, cx_panel_wb;
extern int cx_panel_rect[7][4];                     /* x, y, ancho, alto de cada zona */
void cxpanel_start(void);
extern u32 cx_panel_font[5];
void cxpanel_hidebox(int x, int y, int w, int h);   /* oculta lo nativo que quepa en la caja */
void cxpanel_hideid(u32 id);                        /* oculta el widget nativo con ese ID */
void cxpanel_hidevt(u32 vt);                        /* oculta los widgets nativos de esa clase */
void cxpanel_keepbox(int x, int y, int w, int h);   /* con alguna, sólo queda lo que toca una de estas cajas */
void cxpanel_native(u32 widget);                    /* un widget nativo que la interfaz acaba de tocar */
void cxpanel_view(const char *name, int opening);   /* tras la openView/closeView original */
void cxpanel_hook(int h);                           /* desde cualquier envoltura; h: quién llama */
int cxpanel_press(int x, int y);                    /* 1: el toque es del panel */
int cxpanel_base(void);                             /* 1: se ve la pantalla de grabación de video, sin nada encima */
void cxpanel_update(void);                          /* vuelve a decidir si el panel se ve */
/* cxobj.c: motor del panel. k: 0 iris, 1 obturación. */
int cxobj_panel_ready(void);
u32 cxobj_panel_get(u32 *fno, u32 *sn, u32 *sd, int *gain_db, int *gain_idx);
void cxobj_panel_manual(int k);
void cxobj_panel_auto(int k);
void cxobj_panel_step(int k, int dir);
void cxobj_panel_gain(int idx);
u32 cxobj_panel_wb_get(void);                       /* VALUEID_WB_* pedido */
void cxobj_panel_wb(u32 mode);
void cxobj_panel_onepush(void);                     /* fija el blanco con la carta gris */
extern int cx_textlog;

/* Configuración de cxmode.txt que usa cxobj.c (la lee cxview.c). */
extern int cx_objmanual;
/* cxobj.c: 1 desde que se mandó EXP_MODE_MANUAL hasta que pasa otro modo. */
extern volatile int cx_manual_active;
extern int cx_holdmanual;
extern int cx_gainshift;
extern int cx_expogain;
extern int cx_autounlock;
extern int cx_gainlock;
/* cxobj.c: qué está manual de verdad en la cámara (con holdmanual). */
extern volatile int cx_iris_held, cx_shutter_held;
/* cxobj.c: pone el iris (iris=1) o la obturación en automático o manual en la cámara. */
void cxobj_drive(int iris, int manual);
/* cxobj.c: manda lo pendiente tras un cambio de prioridad (soltar el AE lock y
 * volver a fijar el otro control). Se llama desde el hilo de la interfaz. */
void cxobj_tick(void);
