/*
 * Cuadros completos en la LCD, y la sonda que lo comprueba.
 *
 * Receta de docs/research/dibujar-cuadros.md (análisis estático; la sonda es su
 * primera prueba): un GEN_Icon propio colgado de A_PHYSICAL_LAYER, una imagen de
 * 320x200 pedida al gestor de recursos (bitmap GRM de 16 bits con alfa, el
 * formato del OSD) puesta en su Sprite con un rectángulo que la escala, y por
 * cada cuadro: bloquear el bitmap, escribir los píxeles, soltarlo y avisarle al
 * sprite que cambió. Todo en el hilo de la interfaz. Lo usan la sonda y DOOM
 * (cxdoom.c).
 *
 * Con «probe N» en cxmode.txt:
 *  - al arrancar se anotan /proc/mounts, /proc/meminfo y el espacio libre de /log;
 *  - un toque en la esquina de arriba a la derecha de la pantalla de grabación
 *    mide cuánta memoria se puede pedir (hasta 8 MiB), si se puede correr código
 *    cargado en memoria y cuánto tarda leer de /log, y
 *    muestra el cuadro de prueba: ocho franjas que dicen el orden de los colores
 *    y una barra que se mueve para ver si los cuadros se actualizan sin
 *    parpadeo. Otro toque lo quita.
 *  - probe 1 invalida con Sprite::setImageDirty; probe 2, alternando el número de
 *    rectángulos (por si lo primero parpadea); probe 3 sólo avisa en los dos
 *    primeros cuadros: si la barra sigue avanzando, la pantalla relee el bitmap
 *    por su cuenta y se podría pintar desde otro hilo.
 */
#include "cx.h"

#ifndef CX_FRAME_LIB
#define CX_FRAME_LIB "/libObj.so"
#endif
#ifndef CX_PROBE_FILE
#define CX_PROBE_FILE "/log/PMHOME.EXE"     /* el instalador de PlayMemories: 17 MB */
#endif

enum { F_ICON, F_WSIZE, F_GENID, F_LAYER, F_POS, F_PASSIVE, F_LAST, F_NEXT, F_MODE,
       F_CIP, F_CREATE, F_IMG, F_RESID, F_BMP, F_LOCK, F_UNLOCK, F_ROWPIX,
       F_SETIMG, F_DIRTY, F_CURRECT, F_RPOS, F_ROFF, F_RSIZE, F_RSIZE6, F_FILTER,
       F_RM, NFN };
static const char *const sym[NFN] = {
    "_ZN8GEN_IconC1Ev",                                                  /* ARM */
    "_ZN2ux6wgtsys6Widget7setSizeENS_4core7Fixed32ES3_",
    "_ZN2ux4core2ID5genIDEPKc",
    "_ZN2ux6wgtsys6Widget10setLayerIDERKNS_4core2IDE",
    "_ZN2ux6wgtsys6Widget11setLocalPosERKNS_4core7Vector2E",
    "_ZN2ux6wgtsys6Widget16setPassiveRegionEb",
    "_ZN2ux6wgtsys6Widget8moveLastEv",
    "_ZNK2ux6wgtsys6Widget14getNextSiblingEv",
    "_ZN2ux6wgtsys6Widget7setModeENS1_10WidgetModeE",
    "_ZN2ux6resmgr16CreateImageParamC1Ettj",
    "_ZN2ux6resmgr19ResourceManagerImpl11createImageERKNS0_16CreateImageParamEPNS0_9ImageDataE",
    "_ZN2ux6resmgr9ImageDataC1Ev",
    "_ZNK2ux6resmgr9ImageData8getResIDEv",
    "_ZN2ux6resmgr9ImageData16sys_getImageDataEv",
    "GRM_bitmapWriteLockBits",
    "GRM_bitmapWriteUnlockBits",
    "GRM_bitmapGetRowPixelCount",
    "_ZN2ux6gfxeng6Sprite8setImageEjh",
    "_ZN2ux6gfxeng6Sprite13setImageDirtyEv",
    "_ZN2ux6gfxeng6Sprite13setCurRectNumEt",
    "_ZN2ux6gfxeng6Sprite10setRectPosEtRKNS_4core7Vector2E",
    "_ZN2ux6gfxeng6Sprite18setRectImageOffsetEtNS_4core7Fixed32ES3_",
    "_ZN2ux6gfxeng6Sprite11setRectSizeEtNS_4core7Fixed32ES3_",
    "_ZN2ux6gfxeng6Sprite11setRectSizeEtNS_4core7Fixed32ES3_S3_S3_b",
    "_ZN2ux6gfxeng6Sprite14setImageFilterEb",
    "_ZN2ux6resmgr15ResourceManager10s_instanceE",                        /* dato */
};
static u32 fn[NFN];

#define ICON_BYTES 0x1f0
#define ICON_SPRITE 0x134
#define W_MODE 0x19
#define FMT_OSD 0x41006       /* 16 bits con 4 de alfa */
#define SRC_W 320
#define SRC_H 200
/* Dónde se ve: 4:3 en una LCD 16:9 cuyo OSD mide 640x480. */
#define DST_X 80
#define DST_Y 0
#define DST_W 480
#define DST_H 480

int cx_probe;
static u32 icon[ICON_BYTES / 4 + 4] __attribute__((aligned(8)));
static u32 img[2] __attribute__((aligned(8)));   /* ux::resmgr::ImageData */
static u32 bmp, res_id, pitch, last_t;
static int ready, failed, visible, mode_shown, frames, skipped, bar, flip, n_stat, tested, probing;

static void say(const char *what, u32 a, u32 b)
{
    struct line l;
    if (!cx_log_enabled()) return;
    cxl_begin(&l, "PR");
    cxl_lit(&l, " ");
    cxl_lit(&l, what);
    cxl_lit(&l, " ");
    cxl_hex(&l, a);
    cxl_lit(&l, " ");
    cxl_hex(&l, b);
    cxl_end(&l);
}

/* Antes de cada paso que puede tumbar la interfaz: anotado y en el archivo. */
static void step(const char *what)
{
    say(what, 0, 0);
    cx_log_sync();
}

static int resolve(void)
{
    u32 base;
    int i;
    if (fn[NFN - 1]) return 1;
    base = cx_lib_base(CX_FRAME_LIB);
    for (i = 0; i < NFN; i++) {
        fn[i] = cx_dlsym(base, sym[i]);
        if (!fn[i]) { say("falta el simbolo", (u32)i, 0); fn[NFN - 1] = 0; return 0; }
    }
    return 1;
}

static int create(void)
{
    u32 W = (u32)icon, S = W + ICON_SPRITE, id, rm, cip[6];
    u32 rp = 0;
    int xy[2], zero[2] = { 0, 0 };
    unsigned k;

    if (!resolve()) return 0;
    for (k = 0; k < sizeof(icon) / 4; k++) icon[k] = 0;
    for (k = 0; k < 6; k++) cip[k] = 0;
    step("icono");
    ((u32 (*)(u32))fn[F_ICON])(W);
    step("capa");
    id = ((u32 (*)(const char *))fn[F_GENID])("A_PHYSICAL_LAYER");
    ((void (*)(u32, const u32 *))fn[F_LAYER])(W, &id);
    /* Widget::setSize de la base: GEN_Icon::setSize haría que la clase rehiciera
     * los rectángulos del sprite a su manera. */
    step("medida");
    ((void (*)(u32, int, int))fn[F_WSIZE])(W, DST_W << 16, DST_H << 16);
    xy[0] = DST_X << 16;
    xy[1] = DST_Y << 16;
    ((void (*)(u32, const int *))fn[F_POS])(W, xy);
    ((void (*)(u32, int))fn[F_PASSIVE])(W, 1);
    mode_shown = ((const u8 *)icon)[W_MODE];

    step("imagen");
    ((void (*)(u32 *))fn[F_IMG])(img);
    ((void (*)(u32 *, u32, u32, u32))fn[F_CIP])(cip, SRC_W, SRC_H, FMT_OSD);
    rm = *(const u32 *)fn[F_RM];
    if (!rm || !((int (*)(u32, const u32 *, u32 *))fn[F_CREATE])(rm, cip, img)) {
        say("createImage dijo que no, rm", rm, 0);
        return 0;
    }
    res_id = ((u32 (*)(u32 *))fn[F_RESID])(img);
    bmp = ((u32 (*)(u32 *))fn[F_BMP])(img);
    say("imagen id, bitmap", res_id, bmp);
    cx_log_sync();
    if (res_id == 0xffffffffu || !bmp) return 0;
    ((int (*)(u32, u32 *))fn[F_ROWPIX])(bmp, &rp);
    pitch = rp & 0xffff;
    say("pixeles por fila", pitch, 0);
    if (pitch < SRC_W) return 0;

    step("rectangulo");
    ((void (*)(u32, u32))fn[F_CURRECT])(S, 1);
    ((void (*)(u32, u32, const int *))fn[F_RPOS])(S, 0, zero);
    ((void (*)(u32, u32, int, int))fn[F_ROFF])(S, 0, 0, 0);
    ((void (*)(u32, u32, int, int, int, int, int))fn[F_RSIZE6])
        (S, 0, DST_W << 16, DST_H << 16, SRC_W << 16, SRC_H << 16, 0);
    ((void (*)(u32, u32, int, int))fn[F_RSIZE])(S, 1, 0, 0);
    ((void (*)(u32, int))fn[F_FILTER])(S, 0);
    step("sprite");
    ((void (*)(u32, u32, u32))fn[F_SETIMG])(S, res_id, 0x80);
    say("creado", 0, 0);
    cx_log_sync();
    return 1;
}

/* Crea el cuadro si hace falta y lo pone encima de todo. 0 si no se puede. */
int cxframe_open(void)
{
    if (failed) return 0;
    if (!ready) {
        ready = create();
        if (!ready) { failed = 1; say("no se pudo crear el cuadro", 0, 0); return 0; }
    } else if (!visible) {
        ((void (*)(u32, int))fn[F_MODE])((u32)icon, mode_shown);
        ((void (*)(u32))fn[F_LAST])((u32)icon);
    }
    visible = 1;
    return 1;
}

void cxframe_close(void)
{
    probing = 0;
    if (!ready || !visible) return;
    visible = 0;
    ((void (*)(u32, int))fn[F_MODE])((u32)icon, 3 /* oculto */);
}

/*
 * Lo que la interfaz crea después queda encima (la pantalla de grabación se
 * rearma sola al cerrar un menú): si el cuadro ya no es el último de su capa,
 * vuelve al frente. 1 si hubo que moverlo. Igual que hace el panel con sus textos.
 */
int cxframe_front(void)
{
    if (!ready || !visible || !((u32 (*)(u32))fn[F_NEXT])((u32)icon)) return 0;
    ((void (*)(u32))fn[F_LAST])((u32)icon);
    return 1;
}

/* Para la bitácora: el modo del widget (0 normal, 3 oculto) y, en el bit 8, si se pidió verlo. */
u32 cxframe_state(void) { return ((const u8 *)icon)[W_MODE] | (u32)visible << 8 | (u32)ready << 9; }

/* Los píxeles del cuadro (320x200, `*row` píxeles por fila), o 0 si ahora no se puede. */
u16 *cxframe_lock(u32 *row, int wait)
{
    void *p = 0;
    if (!ready || !visible) return 0;
    if (((int (*)(u32, int, void **))fn[F_LOCK])(bmp, wait, &p) != 0 || !p) { skipped++; return 0; }
    *row = pitch;
    return p;
}

/* Suelta el bitmap; con `changed`, el sprite lo vuelve a subir a la pantalla. */
void cxframe_unlock(int changed)
{
    u32 S = (u32)icon + ICON_SPRITE;
    ((int (*)(u32))fn[F_UNLOCK])(bmp);
    if (!changed) return;
    frames++;
    if (cx_probe == 3 && probing && frames > 2) return;   /* ¿se ve el cambio sin avisarle al sprite? */
    cxframe_front();
    if (cx_probe == 2) {
        flip ^= 1;
        ((void (*)(u32, u32))fn[F_CURRECT])(S, flip ? 2 : 1);
    } else {
        ((void (*)(u32))fn[F_DIRTY])(S);
        ((void (*)(u32, u32, u32))fn[F_SETIMG])(S, res_id, 0x80);
    }
}

/*
 * El cuadro de prueba. Ocho franjas de 40 píxeles: si el formato es R G B A de
 * alto a bajo salen rojo, verde, azul, blanco, negro, transparente, gris a medias
 * y transparente (docs/research/dibujar-cuadros.md, 7.4). Encima, una barra
 * blanca que avanza 4 píxeles por cuadro.
 */
static void paint(void)
{
    static const u16 val[8] = { 0xF00F, 0x0F0F, 0x00FF, 0xFFFF, 0x000F, 0xF000, 0x8888, 0xF0F0 };
    u32 row;
    u16 *d = cxframe_lock(&row, 0);
    int x, y, s;
    if (!d) return;
    for (y = 0; y < SRC_H; y++, d += row) {
        for (s = 0, x = 0; s < 8; s++) {
            int end = x + 40;
            for (; x < end; x++) d[x] = val[s];
        }
        if (y >= 80 && y < 120)
            for (x = bar; x < bar + 4; x++) d[x] = 0xFFFF;
    }
    bar += 4;
    if (bar > SRC_W - 4) bar = 0;
    cxframe_unlock(1);
}

/* Las franjas una vez, sin barra que se mueva (pantalla de carga de DOOM). */
void cxframe_pattern(void) { paint(); }

/* Todo transparente: el cuadro sigue «visible» pero no tapa nada. */
void cxframe_clear(void)
{
    u32 row;
    u16 *d = cxframe_lock(&row, -1);
    int x, y;
    if (!d) return;
    for (y = 0; y < SRC_H; y++, d += row)
        for (x = 0; x < SRC_W; x++) d[x] = 0;
    cxframe_unlock(1);
}

/*
 * El cuadro creado, encima de todo y transparente. Hay que llamarlo desde un toque
 * o desde openView/closeView: creado o mostrado desde los turnos periódicos
 * (getModelInfo, setString) no se vio nunca, aunque después recibiera cuadros
 * (v32 a v34, 2026-11-27); creado al tocar, sí (la sonda). Cambiar su contenido
 * desde los turnos sí funciona.
 */
int cxframe_prepare(void)
{
    if (!cxframe_open()) return 0;
    ((void (*)(u32))fn[F_LAST])((u32)icon);
    cxframe_clear();
    return 1;
}

int cxframe_ready(void) { return ready && visible; }

/* --- sonda: archivos de /proc, espacio de /log y memoria ----------------- */

/* Anota las primeras líneas de un archivo de texto. */
static void dump(const char *path, int lines)
{
    char b[1800];
    struct line l;
    long fd = cx_open(path, O_RDONLY, 0), n, i, start = 0;
    if (fd < 0) { say(path, (u32)fd, 0); return; }
    n = cx_read(fd, b, sizeof(b) - 1);
    cx_close(fd);
    for (i = 0; i < n && lines > 0; i++) {
        if (b[i] != '\n') continue;
        b[i] = 0;
        cxl_begin(&l, "PR");
        cxl_lit(&l, " ");
        cxl_str(&l, b + start, 150);
        cxl_end(&l);
        start = i + 1;
        lines--;
    }
}

/* Número que sigue a `key` en /proc/meminfo, en kB, o 0. */
static u32 meminfo(const char *key)
{
    char b[600];
    long fd = cx_open("/proc/meminfo", O_RDONLY, 0), n, i;
    u32 v = 0;
    int k;
    if (fd < 0) return 0;
    n = cx_read(fd, b, sizeof(b) - 1);
    cx_close(fd);
    if (n < 0) n = 0;
    b[n] = 0;
    for (i = 0; i < n; i++) {
        for (k = 0; key[k] && b[i + k] == key[k]; k++) ;
        if (key[k]) continue;
        for (i += k; b[i] == ' '; i++) ;
        for (; b[i] >= '0' && b[i] <= '9'; i++) v = v * 10 + (u32)(b[i] - '0');
        break;
    }
    return v;
}

/* Memoria que se puede usar sin apretar al sistema: la libre más la caché, en kB. */
u32 cxframe_free_kb(void) { return meminfo("MemFree:") + meminfo("Cached:"); }

/*
 * ¿Cuánta memoria anónima se puede pedir y usar? Pide 8 MiB y los va tocando de
 * a 1 MiB, y se detiene si lo libre más la caché baja de 12 MiB. De paso, con
 * esa memoria: cuánto tarda leer 1 MiB de /log, y si en la dirección donde se
 * cargará DOOM se puede poner código y correrlo.
 */
static void memtest(void)
{
    u32 mb, off, f0, f1, t0, t1;
    long a, fd, c;
    say("memoria libre, cache (kB)", meminfo("MemFree:"), meminfo("Cached:"));
    a = sys6(SYS_mmap2, 0, 8 << 20, 3 /* lectura y escritura */, 0x22 /* privada y anónima */, -1, 0);
    if ((u32)a > 0xfffff000u) { say("mmap de 8 MiB fallo", (u32)a, 0); return; }
    for (mb = 0; mb < 8; mb++) {
        if (meminfo("MemFree:") + meminfo("Cached:") < 12 * 1024) break;
        for (off = 0; off < (1 << 20); off += 4096) ((volatile u8 *)a)[(mb << 20) + off] = 1;
    }
    say("MiB usados de 8, direccion", mb, (u32)a);
    say("memoria libre, cache (kB)", meminfo("MemFree:"), meminfo("Cached:"));
    fd = cx_open(CX_PROBE_FILE, O_RDONLY, 0);
    if (fd >= 0 && mb) {
        long got = 0, n;
        t0 = cx_now(&f0);
        while (got < (1 << 20) && (n = cx_read(fd, (void *)(a + got), (1 << 20) - got)) > 0) got += n;
        t1 = cx_now(&f1);
        cx_close(fd);
        say("leidos de /log (bytes), en ms", (u32)got, (t1 - t0) * 954 + f1 - f0);
    } else {
        say("sin archivo de prueba en /log", (u32)fd, 0);
    }
    sys3(91 /* munmap */, a, 8 << 20, 0);
    cx_log_sync();

    /* Una página con «mov r0, #42; bx lr»: ¿se puede correr código cargado así? */
    step("codigo");
    c = sys6(SYS_mmap2, 0, 4096, 7 /* leer, escribir y ejecutar */, 0x22, -1, 0);
    if ((u32)c > 0xfffff000u) {
        say("sin pagina para codigo", (u32)c, 0);
    } else {
        ((u32 *)c)[0] = 0xe3a0002a;
        ((u32 *)c)[1] = 0xe12fff1e;
        sys3(0xf0002 /* cacheflush */, c, c + 8, 0);
        say("codigo cargado dio", ((u32 (*)(void))c)(), (u32)c);
        sys3(91, c, 4096, 0);
    }
    cx_log_sync();
}

void cxframe_start(void)
{
    u32 st[20];
    unsigned k;
    if (!cx_probe) return;
    dump("/proc/mounts", 24);
    dump("/proc/meminfo", 6);
    for (k = 0; k < 20; k++) st[k] = 0;
    if (sys3(99 /* statfs */, "/log", st, 0) == 0) say("/log bloque, bloques libres", st[1], st[4]);
    else say("/log sin statfs", 0, 0);
}

/* El toque en la esquina: la primera vez prueba la memoria; pone y quita el cuadro. */
void cxframe_toggle(void)
{
    if (!cx_probe || failed || cxdoom_active()) return;
    if (!tested) { tested = 1; memtest(); }
    if (!probing) {
        if (!cxframe_open()) return;
        probing = 1;
        paint();
    } else {
        probing = 0;
        cxframe_close();
    }
    say(probing ? "mostrar" : "ocultar", (u32)frames, (u32)skipped);
}

/* Desde el hilo de la interfaz, unas 80 veces por segundo: un cuadro cada 33 ms. */
void cxframe_tick(void)
{
    u32 frac, t;
    if (!probing || cxdoom_active()) return;
    t = cx_now(&frac) * 954 + frac;      /* frac va de 0 a 953 */
    if (t - last_t < 32) return;
    last_t = t;
    paint();
    if (!(frames & 127) && n_stat < 8) { n_stat++; say("cuadros, saltados", (u32)frames, (u32)skipped); }
}
