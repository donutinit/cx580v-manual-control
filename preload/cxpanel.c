/*
 * Panel propio en la pantalla de grabación: iris, obturación y ganancia a la
 * vista, cada uno con su automático, y se ajustan tocando, sin entrar al menú.
 *
 *                                                   [ + ]   orilla derecha,
 *                                                   AUTO    donde iban W y T
 *                                                   [ - ]
 *     F2.6    1/180    [12dB]                               abajo
 *
 * - Tocar un valor lo elige (queda entre corchetes). AUTO, o tocar otra vez el
 *   valor elegido, lo pasa de automático a manual o al revés. En automático
 *   lleva una A al final; la ganancia automática dice AGC y los dB que avisa la
 *   cámara.
 * - «+» y «-» mueven el valor elegido. El primer toque sobre un control en
 *   automático sólo lo fija donde está, como en las pantallas de fábrica.
 *
 * Cómo se dibuja (docs/research/ux-panel-propio.md, receta A1; visto en la
 * cámara el 2026-11-27): cinco GEN_Label propias, en memoria estática, colgadas
 * de la capa A_PHYSICAL_LAYER, con la fuente medida en las etiquetas de fábrica.
 * El tacto llega por cxtouch.c, que se traga los toques que caen en el panel.
 * Qué pantalla está abierta se sigue con openView/closeView: el panel sólo se ve
 * y sólo atiende sobre la pantalla de grabación de video.
 *
 * Lo nativo que estorba (botones W, T y grabar; indicadores de obturación y AE
 * Shift) se oculta: se recorre el árbol de widgets de la capa y se pone en modo
 * oculto lo que cae entero dentro de una caja (hidebox) o tiene cierto ID
 * (hidew). Campos de ux::wgtsys::Widget leídos del desensamblado de libObj.so
 * 1.07: modo +0x19, posición local +0x3c/+0x40, ancho y alto +0x78/+0x7c (los
 * cuatro en 16.16), ID +0x104.
 *
 * Todo lo que toca el framework ux corre en el hilo de la interfaz (se anota en
 * la primera openView y se compara en cada llamada). Las etiquetas se crean, se
 * muestran y se ocultan sólo después de una openView/closeView original o dentro
 * de un toque, nunca en medio de un recorrido de widgets.
 *
 * cxmode.txt:  panel 1              sólo dibuja
 *              panel 2              dibuja y atiende el tacto
 *              panelz N X Y W H     zona N (0 «-», 1 iris, 2 obturación, 3 ganancia,
 *                                   4 «+», 5 AUTO, 6 balance) en el espacio de 640x480
 *              panelwb 1            lectura de balance de blancos (preajustes y One Push)
 *              panelfont N HEX      N: 0 fuente, 1 tamaño, 2 estilo, 3 alineación, 4 dirección
 *              paneldraw 0          no llamar a TextSprite::drawText a mano
 *              paneldump 1          anota el árbol de widgets en el primer toque
 *              hidebox X Y W H      oculta lo nativo que quepa entero en esa caja
 *              hidew HEX            oculta el widget con ese ID
 *              hidevt HEX           oculta los widgets de esa clase (tabla virtual menos
 *                                   la base de libObj.so, como sale en las líneas PW)
 *              keepbox X Y W H      con alguna keepbox, de la pantalla de grabación sólo
 *                                   queda lo que toca una de esas cajas; lo demás se oculta
 */
#include "cx.h"

#ifndef CX_PANEL_LIB
#define CX_PANEL_LIB "/libObj.so"
#endif

enum { F_CTOR, F_SIZE, F_FONT, F_FSIZE, F_FSTYLE, F_HALIGN, F_DIR, F_STR, F_DRAW, F_GENID, F_LAYER, F_POS,
       F_LAST, F_NEXT, F_MODE, F_PASSIVE, F_PARENT, F_CHILD, NFN };
static const char *const sym[NFN] = {
    "_ZN9GEN_LabelC1Ev",
    "_ZN9GEN_Label7setSizeEN2ux4core7Fixed32ES2_",
    "_ZN9GEN_Label7setFontEj",
    "_ZN9GEN_Label11setFontSizeEj",
    "_ZN9GEN_Label12setFontStyleEj",
    "_ZN9GEN_Label18setHorizontalAlignEj",
    "_ZN9GEN_Label16setDirectionTypeEj",
    "_ZN2ux6gfxeng10TextSprite9setStringEPKc",
    "_ZN2ux6gfxeng10TextSprite8drawTextEv",
    "_ZN2ux4core2ID5genIDEPKc",
    "_ZN2ux6wgtsys6Widget10setLayerIDERKNS_4core2IDE",
    "_ZN2ux6wgtsys6Widget11setLocalPosERKNS_4core7Vector2E",
    "_ZN2ux6wgtsys6Widget8moveLastEv",
    "_ZNK2ux6wgtsys6Widget14getNextSiblingEv",
    "_ZN2ux6wgtsys6Widget7setModeENS1_10WidgetModeE",
    "_ZN2ux6wgtsys6Widget16setPassiveRegionEb",
    "_ZNK2ux6wgtsys6Widget9getParentEv",
    "_ZNK2ux6wgtsys6Widget13getFirstChildEv",
};
static u32 fn[NFN], lib_base;

#define NLAB 7                /* la séptima (balance) sólo con panelwb */
#define LABEL_BYTES 0x290
#define SPRITE 0x144          /* el TextSprite dentro de la GEN_Label */
#define DIRTY 0x285
#define W_MODE 0x19
#define W_X 0x3c
#define W_Y 0x40
#define W_W 0x78
#define W_H 0x7c
#define W_ID 0x104
#define MODE_HIDDEN 3
#define TEXT_MAX 16
#define NHOOK 6
#define NBOX 4
#define NKEEP 6
#define NHIDEID 12
#define NHIDEVT 8
#define NHID 80
#define QUIET_SECS 8
#define MAX_DEPTH 12
#define MAX_WALK 900
#define MAX_DUMP 440
#define NTOP 8

int cx_panel, cx_panel_draw = 1, cx_panel_dump, cx_panel_wb;
/* x, y, ancho y alto de: «-», iris, obturación, ganancia, «+», AUTO, balance. */
int cx_panel_rect[NLAB][4] = {
    { 572, 290, 68, 90 },
    { 110, 418, 100, 52 },
    { 210, 418, 120, 52 },
    { 330, 418, 100, 52 },
    { 572, 110, 68, 90 },
    { 572, 205, 68, 80 },
    { 4, 418, 104, 52 },             /* balance de blancos, donde iba el botón de ver imágenes */
};
u32 cx_panel_font[5] = { 0x2038000, 0x203e010, 0x203e003, 0x2036029, 0x2038002 };

static u32 mem[NLAB][LABEL_BYTES / 4 + 4] __attribute__((aligned(8)));
static char cur[NLAB][TEXT_MAX];
static int created, failed, shown, mode_shown;
static int sel;                      /* 0 iris, 1 obturación, 2 ganancia, 3 balance */
static int nlab = NLAB - 1;          /* etiquetas en uso */
static int wb_pre = 1;               /* preajuste al que vuelve AUTO desde el automático */
static u32 wb_set_sec;                /* hasta cuándo la lectura dice SET tras fijar con la carta */
#define V_WB_AUTO 0xc7
#define V_WB_DAYLIGHT 0xc8
#define V_WB_INCANDESCENT 0xcf
#define V_WB_ONEPUSH 0xc3
static u32 ui_tid, seen_seq = 0xffffffffu;
static int force;
static int base_movie, depth;
static u32 stack[6];
static u32 n_all[NHOOK], n_ui[NHOOK], stat_sec, n_stat;
static int box[NBOX][4], nbox;
static int keep[NKEEP][4], nkeep;
static u32 hideid[NHIDEID], hidevt[NHIDEVT];
static int nhideid, nhidevt;
/* Mientras hay un aviso encima (CAUTION, cambio de modo) no se busca qué más
 * ocultar: sus piezas no son de la pantalla de grabación. */
static u32 quiet_until;
static struct { u32 w, id, vt; } hid[NHID];
static int nhid, need_scan, dumped, n_hidelog, n_fight;
static u32 scan_sec;
/* Raíces de las vistas nativas vistas desde la última apertura o cierre de pantalla. */
static u32 tops[NTOP];
static int ntops, n_toplog;

static u32 gettid(void) { return (u32)sys3(224 /* gettid */, 0, 0, 0); }

/*
 * Lo que NO sirve para saber qué pantalla está a la vista (v26, 2026-11-27):
 * contar las vistas vivas con el constructor y el destructor de ViewBase. El
 * menú principal sigue vivo después de cerrarse, así que se aprendía como «de
 * casa» y, con el menú abierto, el panel creía estar sobre la pantalla de
 * grabación y le ocultaba los botones al menú.
 */
void cxpanel_start(void)
{
}

void cxpanel_hidebox(int x, int y, int w, int h)
{
    if (nbox >= NBOX) return;
    box[nbox][0] = x; box[nbox][1] = y; box[nbox][2] = w; box[nbox][3] = h;
    nbox++;
}

void cxpanel_hideid(u32 id)
{
    if (id && nhideid < NHIDEID) hideid[nhideid++] = id;
}

void cxpanel_hidevt(u32 vt)
{
    if (vt && nhidevt < NHIDEVT) hidevt[nhidevt++] = vt;
}

void cxpanel_keepbox(int x, int y, int w, int h)
{
    if (nkeep >= NKEEP) return;
    keep[nkeep][0] = x; keep[nkeep][1] = y; keep[nkeep][2] = w; keep[nkeep][3] = h;
    nkeep++;
}

static void say(const char *what, u32 a, u32 b)
{
    struct line l;
    if (!cx_log_enabled()) return;
    cxl_begin(&l, "PN");
    cxl_lit(&l, " ");
    cxl_lit(&l, what);
    cxl_lit(&l, " ");
    cxl_hex(&l, a);
    cxl_lit(&l, " ");
    cxl_hex(&l, b);
    cxl_end(&l);
}

static int resolve(void)
{
    int i;
    if (fn[NFN - 1]) return 1;
    lib_base = cx_lib_base(CX_PANEL_LIB);
    for (i = 0; i < NFN; i++) {
        fn[i] = cx_dlsym(lib_base, sym[i]);
        if (!fn[i]) { say("falta el simbolo", (u32)i, 0); fn[NFN - 1] = 0; return 0; }
    }
    return 1;
}

/* Antes de cada paso que puede tumbar la interfaz: queda anotado y en el archivo. */
static void step(const char *what, int i)
{
    say(what, (u32)i, (u32)mem[i]);
    if (i == 0) cx_log_sync();
}

static void make(int i)
{
    u32 L = (u32)mem[i], id;
    int xy[2];
    unsigned k;
    for (k = 0; k < sizeof(mem[i]) / 4; k++) mem[i][k] = 0;
    step("crear", i);
    ((void (*)(u32))fn[F_CTOR])(L);
    step("capa", i);
    id = ((u32 (*)(const char *))fn[F_GENID])("A_PHYSICAL_LAYER");
    ((void (*)(u32, const u32 *))fn[F_LAYER])(L, &id);
    step("medida", i);
    ((void (*)(u32, int, int))fn[F_SIZE])(L, cx_panel_rect[i][2] << 16, cx_panel_rect[i][3] << 16);
    step("fuente", i);
    ((void (*)(u32, u32))fn[F_FONT])(L, cx_panel_font[0]);
    ((void (*)(u32, u32))fn[F_FSIZE])(L, cx_panel_font[1]);
    ((void (*)(u32, u32))fn[F_FSTYLE])(L, cx_panel_font[2]);
    ((void (*)(u32, u32))fn[F_HALIGN])(L, cx_panel_font[3]);
    ((void (*)(u32, u32))fn[F_DIR])(L, cx_panel_font[4]);
    step("lugar", i);
    xy[0] = cx_panel_rect[i][0] << 16;
    xy[1] = cx_panel_rect[i][1] << 16;
    ((void (*)(u32, const int *))fn[F_POS])(L, xy);
    step("pasiva", i);
    ((void (*)(u32, int))fn[F_PASSIVE])(L, 1);
}

static void create(void)
{
    int i;
    if (!resolve()) { failed = 1; return; }
    nlab = cx_panel_wb ? NLAB : NLAB - 1;
    for (i = 0; i < nlab; i++) make(i);
    mode_shown = ((const u8 *)mem[0])[W_MODE];
    created = shown = 1;
    force = 1;
    say("creado, modo", (u32)mode_shown, ui_tid);
    cx_log_sync();
}

static void text(int i, const char *s)
{
    u32 L = (u32)mem[i];
    int k, same = 1;
    for (k = 0; k < TEXT_MAX - 1 && s[k]; k++)
        if (cur[i][k] != s[k]) same = 0;
    if (same && !cur[i][k] && !force) return;
    for (k = 0; k < TEXT_MAX - 1 && s[k]; k++) cur[i][k] = s[k];
    cur[i][k] = 0;
    ((void (*)(u32, const char *))fn[F_STR])(L + SPRITE, cur[i]);
    ((u8 *)L)[DIRTY] = 1;
    if (cx_panel_draw) ((int (*)(u32))fn[F_DRAW])(L + SPRITE);
}

static char *put(char *p, const char *s) { while (*s) *p++ = *s++; return p; }

/* Decimal sin división (no hay libgcc). Hasta 99999. */
static char *dec(char *p, u32 v)
{
    static const u32 pw10[5] = { 10000, 1000, 100, 10, 1 };
    int i, started = 0;
    if (v > 99999) v = 99999;
    for (i = 0; i < 5; i++) {
        int d = 0;
        while (v >= pw10[i]) { v -= pw10[i]; d++; }
        if (d || started || i == 4) { *p++ = (char)('0' + d); started = 1; }
    }
    return p;
}

/* Índice de ganancia (0..8, de 3 en 3 dB) más cercano por abajo a `db`. */
static int gain_index(int db)
{
    int i = 0;
    while (db >= 3 && i < 8) { db -= 3; i++; }
    return i;
}

static void refresh(void)
{
    u32 fno, sn, sd, seq;
    int g, gi;
    char b[TEXT_MAX + 8], *p;
    seq = cxobj_panel_get(&fno, &sn, &sd, &g, &gi);
    if (wb_set_sec && cx_now(0) >= wb_set_sec) { wb_set_sec = 0; force = 1; }   /* se acabó el «OK» */
    if (seq == seen_seq && !force) return;
    seen_seq = seq;

    text(0, "-");
    text(4, "+");
    if (!cx_panel_wb) text(5, "AUTO");

    p = b;
    if (sel == 0) *p++ = '[';
    *p++ = 'F';
    if (fno) {
        u32 whole = 0, tenth = 0;
        while (fno >= 100) { fno -= 100; whole++; }
        while (fno >= 10) { fno -= 10; tenth++; }
        p = dec(p, whole);
        if (whole < 10) { *p++ = '.'; *p++ = (char)('0' + tenth); }
    } else {
        p = put(p, "--");
    }
    if (!cx_iris_held) *p++ = 'A';
    if (sel == 0) *p++ = ']';
    *p = 0;
    text(1, b);

    p = b;
    if (sel == 1) *p++ = '[';
    if (sd) { p = dec(p, sn); *p++ = '/'; p = dec(p, sd); }
    else p = put(p, "--");
    if (!cx_shutter_held) *p++ = 'A';
    if (sel == 1) *p++ = ']';
    *p = 0;
    text(2, b);

    p = b;
    if (sel == 2) *p++ = '[';
    if (gi >= 0) { p = dec(p, (u32)(gi * 3)); p = put(p, "dB"); }
    else { p = put(p, "AGC"); if (g >= 0) p = dec(p, (u32)g); }
    if (sel == 2) *p++ = ']';
    *p = 0;
    text(3, b);

    if (cx_panel_wb) {
        u32 m = cxobj_panel_wb_get();
        text(5, sel == 3 && m == V_WB_ONEPUSH ? "SET" : "AUTO");
        p = b;
        if (sel == 3) *p++ = '[';
        p = put(p, wb_set_sec ? "OK" : m == V_WB_AUTO ? "AWB" : m == V_WB_DAYLIGHT ? "SOL" :
                   m == V_WB_INCANDESCENT ? "INT" : m == V_WB_ONEPUSH ? "1P" : "WB");
        if (sel == 3) *p++ = ']';
        *p = 0;
        text(6, b);
    }
    force = 0;
}

/* --- lo nativo que estorba ---------------------------------------------- */

static int unwanted(u32 w, int x, int y, int ww, int hh)
{
    u32 id = *(const u32 *)(w + W_ID), vt = *(const u32 *)w - lib_base;
    int i;
    for (i = 0; i < nhideid; i++)
        if (hideid[i] == id) return 1;
    for (i = 0; i < nhidevt; i++)
        if (hidevt[i] == vt) return 1;
    if (ww <= 0 || hh <= 0) return 0;          /* contenedores sin tamaño: se mira adentro */
    for (i = 0; i < nbox; i++)
        if (x >= box[i][0] && y >= box[i][1] && x + ww <= box[i][0] + box[i][2] && y + hh <= box[i][1] + box[i][3])
            return 1;
    if (nkeep) {
        /* «Todo menos»: lo que no toca ninguna caja de las que se conservan, fuera. */
        for (i = 0; i < nkeep; i++)
            if (x < keep[i][0] + keep[i][2] && x + ww > keep[i][0] && y < keep[i][1] + keep[i][3] && y + hh > keep[i][1])
                return 0;
        return 1;
    }
    return 0;
}

static void dump_line(int d, u32 w, int x, int y, int ww, int hh)
{
    struct line l;
    if (!cx_log_enabled()) return;
    cxl_begin(&l, "PW");
    cxl_lit(&l, " ");
    cxl_hex(&l, (u32)d);
    cxl_lit(&l, " ");
    cxl_hex(&l, w);
    cxl_lit(&l, " vt=");
    cxl_hex(&l, *(const u32 *)w - lib_base);
    cxl_lit(&l, " m=");
    cxl_hex(&l, ((const u8 *)w)[W_MODE]);
    cxl_lit(&l, " ");
    cxl_hex(&l, (u32)x);
    cxl_lit(&l, ",");
    cxl_hex(&l, (u32)y);
    cxl_lit(&l, " ");
    cxl_hex(&l, (u32)ww);
    cxl_lit(&l, "x");
    cxl_hex(&l, (u32)hh);
    cxl_lit(&l, " id=");
    cxl_hex(&l, *(const u32 *)(w + W_ID));
    cxl_end(&l);
}

/*
 * Recorre (sin recursión) el subárbol de `w` con la posición absoluta de cada
 * widget. Oculta lo que piden hidebox e hidew y, si dump, lo anota. Con
 * `brothers` sigue también con los hermanos de `w`. Las raíces (profundidad 0)
 * y las etiquetas propias no se tocan.
 */
static int walk(u32 w, int brothers, int dump, int *lines)
{
    u32 up[MAX_DEPTH];
    int ox[MAX_DEPTH], oy[MAX_DEPTH], d = 0, count = 0, i;
    ox[0] = oy[0] = 0;
    while (w && count < MAX_WALK) {
        int mine = 0, gone = 0, x, y, ww, hh;
        u32 c = 0, n;
        count++;
        for (i = 0; i < NLAB; i++)
            if (w == (u32)mem[i]) mine = 1;
        if (!mine) {
            x = ox[d] + (*(const int *)(w + W_X) >> 16);
            y = oy[d] + (*(const int *)(w + W_Y) >> 16);
            ww = *(const int *)(w + W_W) >> 16;
            hh = *(const int *)(w + W_H) >> 16;
            if (dump && *lines < MAX_DUMP) {
                dump_line(d, w, x, y, ww, hh);
                if ((++*lines & 31) == 0) cx_log_sync();   /* el búfer de la bitácora es chico */
            }
            if (d >= 1 && unwanted(w, x, y, ww, hh)) {
                gone = 1;
                if (((const u8 *)w)[W_MODE] != MODE_HIDDEN) ((void (*)(u32, int))fn[F_MODE])(w, MODE_HIDDEN);
                if (nhid < NHID) {
                    hid[nhid].w = w;
                    hid[nhid].id = *(const u32 *)(w + W_ID);
                    hid[nhid].vt = *(const u32 *)w;
                    nhid++;
                }
                if (n_hidelog < 40) { n_hidelog++; say("oculto", *(const u32 *)(w + W_ID), ((u32)x << 16) | ((u32)y & 0xffff)); }
            }
            /* Dentro de algo ya oculto no hace falta mirar (salvo para anotar el mapa). */
            if (d < MAX_DEPTH - 1 && (!gone || dump)) c = ((u32 (*)(u32))fn[F_CHILD])(w);
            if (c) {
                up[d] = w;
                d++;
                ox[d] = x;
                oy[d] = y;
                w = c;
                continue;
            }
        }
        for (;;) {
            if (d == 0 && !brothers) { w = 0; break; }
            n = ((u32 (*)(u32))fn[F_NEXT])(w);
            if (n) { w = n; break; }
            if (d == 0) { w = 0; break; }
            d--;
            w = up[d];
        }
    }
    return count;
}

/*
 * Busca por dónde entrar al árbol. Primero la raíz de la capa: una etiqueta
 * propia cuelga de ella, y el nodo de su padre (+8) apunta a su dueño (+0);
 * Widget::getParent() no sirve porque para un hijo directo de la raíz contesta
 * que no tiene padre (visto en la cámara el 2026-11-27). Si por ahí no sale
 * nada, las raíces de las vistas nativas que anotó cxpanel_native().
 */
static void scan(int dump)
{
    u32 pn, root = 0, first = 0;
    int count = 0, lines = 0, i;
    nhid = 0;
    need_scan = 0;
    if (!created || (!dump && !nbox && !nhideid && !nhidevt && !nkeep)) return;
    pn = *(const u32 *)((u32)mem[0] + 8);
    if (pn && !(pn & 3)) root = *(const u32 *)pn;
    if (root && !(root & 3) && root != (u32)mem[0]) first = ((u32 (*)(u32))fn[F_CHILD])(root);
    if (dump) { say("raiz", pn, root); say("primer hijo, vistas", first, (u32)ntops); }
    if (first) count = walk(first, 1, dump, &lines);
    else for (i = 0; i < ntops; i++) count += walk(tops[i], 0, dump, &lines);
    if (dump) { say("arbol, widgets", (u32)count, (u32)lines); cx_log_sync(); }
}

/*
 * Un widget nativo que la interfaz acaba de tocar (una etiqueta a la que le puso
 * texto, un icono al que le puso imagen). Se sube hasta la raíz de su vista y se
 * anota, para tener por dónde recorrer aunque la raíz de la capa no aparezca.
 */
void cxpanel_native(u32 w)
{
    u32 top = w;
    int i;
    if (!cx_panel || !created || !shown || !w || (w & 3) || ntops >= NTOP) return;
    if (!nbox && !nhideid && !nhidevt && !nkeep && !cx_panel_dump) return;
    if (!ui_tid || gettid() != ui_tid) return;
    for (i = 0; i < NLAB; i++)
        if (w == (u32)mem[i]) return;
    for (i = 0; i < MAX_DEPTH; i++) {
        u32 p = ((u32 (*)(u32))fn[F_PARENT])(top);
        if (!p || (p & 3)) break;
        top = p;
    }
    for (i = 0; i < ntops; i++)
        if (tops[i] == top) return;
    tops[ntops++] = top;
    need_scan = 1;
    if (n_toplog < 12) { n_toplog++; say("vista nativa", top, w); }
}

/*
 * La vista puede volver a mostrar lo que se ocultó: se vuelve a ocultar. Si un
 * widget ya no es el que se anotó (la vista se rearmó por dentro, por ejemplo
 * al cambiar entre HD y STD), no se toca y se pide otro recorrido.
 */
static void enforce(void)
{
    int i;
    for (i = 0; i < nhid; i++) {
        u32 w = hid[i].w;
        if (*(const u32 *)w != hid[i].vt || *(const u32 *)(w + W_ID) != hid[i].id) { nhid = 0; need_scan = 1; return; }
        if (((const u8 *)w)[W_MODE] == MODE_HIDDEN) continue;
        ((void (*)(u32, int))fn[F_MODE])(w, MODE_HIDDEN);
        if (n_fight < 24) { n_fight++; say("volvio, oculto otra vez", hid[i].id, w); }
    }
}

/* Crea, muestra u oculta. Sólo desde openView/closeView y desde el tacto. */
static void apply(void)
{
    int want = cx_panel && base_movie && depth == 0, i;
    if (want && !created && !failed) create();
    if (!created) return;
    if (want != shown) {
        for (i = 0; i < nlab; i++)
            ((void (*)(u32, int))fn[F_MODE])((u32)mem[i], want ? mode_shown : MODE_HIDDEN);
        shown = want;
        force = 1;
        say(want ? "mostrar" : "ocultar", (u32)depth, depth ? stack[depth - 1] : 0);
    }
    if (shown) {
        /* Las vistas que se abrieron después quedan encima: el panel vuelve al frente. */
        for (i = 0; i < nlab; i++)
            if (((u32 (*)(u32))fn[F_NEXT])((u32)mem[i])) ((void (*)(u32))fn[F_LAST])((u32)mem[i]);
        if (cx_now(0) >= quiet_until) scan(0);
        refresh();
    }
}

static int starts(const char *s, const char *prefix)
{
    while (*prefix) if (*s++ != *prefix++) return 0;
    return 1;
}

static int has(const char *s, const char *word)
{
    for (; *s; s++) if (starts(s, word)) return 1;
    return 0;
}

/* Nombre de vista sin el «_L» final: view/IRIS y view/IRIS_L son la misma pantalla. */
static u32 hname(const char *s)
{
    u32 h = 5381;
    int n = 0, i;
    while (s[n]) n++;
    if (n > 2 && s[n - 2] == '_' && s[n - 1] == 'L') n -= 2;
    for (i = 0; i < n; i++) h = h * 33 + (u8)s[i];
    return h ? h : 1;
}

/* Pantallas de fábrica que se ajustan sobre la imagen en vivo (bitácoras del 2026-11-27). */
static int over_image(const char *n)
{
    static const char *const live[] = { "IRIS", "SHUTTER_SPEED", "AE_SHIFT", "WB_SHIFT", "WHITE_BALANCE",
                                        "EXPOSURE", "SPOT_", "FOCUS", "AGC_LIMIT", "TELE_MACRO" };
    unsigned i;
    for (i = 0; i < sizeof(live) / sizeof(live[0]); i++)
        if (starts(n, live[i])) return 1;
    return 0;
}

/*
 * Después de la openView/closeView original. Se lleva una pila con lo que hay
 * encima de la pantalla de grabación; el panel sólo se ve con la pila vacía.
 */
void cxpanel_view(const char *name, int opening)
{
    if (!cx_panel || !name) return;
    if (!ui_tid) ui_tid = gettid();
    if (gettid() != ui_tid) return;
    if (starts(name, "view/POWER_ILLUSTRATION") || starts(name, "view/CAUTION")) {
        /* Un aviso encima de la pantalla de grabación: mientras esté, no se busca
         * qué más ocultar (lo ya oculto se mantiene). Si no avisa que cerró, vence solo. */
        quiet_until = opening ? cx_now(0) + QUIET_SECS : 0;
        need_scan = 1;
        return;
    }
    nhid = 0;                        /* con cada cambio de pantalla los widgets pueden ser otros */
    ntops = 0;
    if (opening) {
        if (starts(name, "view/MOV_REC")) { base_movie = 1; depth = 0; }
        else if (starts(name, "view/STL_REC")) { base_movie = 0; depth = 0; }
        else {
            u32 h = hname(name);
            /* El menú principal siempre es el primero de su cadena, y las pantallas de
             * ajuste sobre la imagen cierran los menús sin avisar. Las demás (listas de
             * opciones como REC_MODE o PB_DEMO_CAM_SET) se abren dentro del menú y al
             * cerrarse se vuelve a él: se apilan encima. */
            if (starts(name, "view/GLOBAL_MENU") || over_image(name + 5)) depth = 0;
            if (!(depth && stack[depth - 1] == h) && depth < 6) stack[depth++] = h;
        }
    } else {
        u32 h = hname(name);
        int j;
        for (j = depth - 1; j >= 0; j--)
            if (stack[j] == h) { depth = j; break; }
    }
    apply();
}

/*
 * Desde cualquier envoltura. Cuenta quién llama y en qué hilo (para saber qué
 * llamadas sirven de reloj) y, en el hilo de la interfaz, refresca los textos y
 * le da su turno al motor de corrección.
 */
void cxpanel_hook(int h)
{
    u32 sec;
    if (!cx_panel) return;
    n_all[h]++;
    if (!ui_tid || gettid() != ui_tid) return;
    n_ui[h]++;
    if (h == 5) return;              /* updateResource: sólo se cuenta, puede correr a medio dibujo */
    /* Dentro de un aviso de la cámara (h = 0) no se mandan órdenes: sólo se pinta. */
    if (h != 0) cxobj_tick();
    if (!created || !shown) return;
    refresh();
    sec = cx_now(0);
    if (h != 0) {
        /* Un recorrido por segundo (la vista se arma y se rearma sin avisar) y, entre
         * recorridos, se vuelve a ocultar lo que la vista haya vuelto a mostrar. */
        if (sec >= quiet_until && (need_scan || sec != scan_sec)) { scan_sec = sec; scan(0); }
        else enforce();
    }
    if (n_stat < 16 && sec - stat_sec >= 3 && cx_log_enabled()) {
        struct line l;
        int i;
        stat_sec = sec;
        n_stat++;
        cxl_begin(&l, "PN");
        cxl_lit(&l, " llamadas");
        for (i = 0; i < NHOOK; i++) {
            cxl_lit(&l, " ");
            cxl_hex(&l, n_ui[i]);
            cxl_lit(&l, "/");
            cxl_hex(&l, n_all[i]);
        }
        cxl_end(&l);
    }
}

/*
 * Balance: «+» y «-» pasan por automático, sol, interior y One Push. Con One
 * Push elegido, el botón de la derecha dice SET y fija el blanco con lo que la
 * cámara tiene enfrente (la carta gris); con los demás dice AUTO y va y vuelve
 * del automático.
 */
static const u32 wb_list[4] = { V_WB_AUTO, V_WB_DAYLIGHT, V_WB_INCANDESCENT, V_WB_ONEPUSH };

static void wb_act(int dir)
{
    u32 m = cxobj_panel_wb_get();
    int i = m == wb_list[1] ? 1 : m == wb_list[2] ? 2 : m == wb_list[3] ? 3 : 0;
    wb_set_sec = 0;
    if (dir == 0) {
        if (i == 3) { cxobj_panel_onepush(); wb_set_sec = cx_now(0) + 2; return; }
        if (i) wb_pre = i;
        cxobj_panel_wb(i ? V_WB_AUTO : wb_list[wb_pre]);
        return;
    }
    i = (i + dir) & 3;
    if (i) wb_pre = i;
    cxobj_panel_wb(wb_list[i]);
}

static void gain_act(int dir)   /* 0: alternar automático; +1/-1: paso */
{
    u32 fno, sn, sd;
    int g, gi;
    cxobj_panel_get(&fno, &sn, &sd, &g, &gi);
    if (gi < 0) cxobj_panel_gain(gain_index(g));          /* se fija en lo que hay ahora */
    else if (dir == 0) cxobj_panel_gain(-1);
    else {
        gi += dir;
        if (gi < 0) gi = 0;
        if (gi > 8) gi = 8;
        cxobj_panel_gain(gi);
    }
}

/* Un toque en (x, y). 1 si cayó en el panel: entonces no pasa a la interfaz. */
int cxpanel_press(int x, int y)
{
    int z, v;
    if (!cx_panel || !ui_tid || gettid() != ui_tid) return 0;
    apply();                         /* un toque también es buen momento para crear o traer al frente */
    if (!created || !shown) return 0;
    if (cx_panel_dump && !dumped) { dumped = 1; scan(1); }
    if (cx_panel < 2) return 0;
    for (z = 0; z < nlab; z++) {
        const int *r = cx_panel_rect[z];
        if (x >= r[0] && x < r[0] + r[2] && y >= r[1] && y < r[1] + r[3]) break;
    }
    if (z == nlab) return 0;
    say("toque", (u32)z, (u32)sel);
    v = z == 6 ? 3 : z - 1;          /* qué valor es esa zona (si es una de valor) */
    if (z != 0 && z != 4 && z != 5 && sel != v) {
        sel = v;
    } else if (z != 0 && z != 4) {
        /* AUTO, o el valor que ya estaba elegido: de automático a manual y al revés. */
        if (sel == 3) wb_act(0);
        else if (sel == 2) gain_act(0);
        else if (sel == 0 ? cx_iris_held : cx_shutter_held) cxobj_panel_auto(sel);
        else cxobj_panel_manual(sel);
    } else {
        int dir = z == 0 ? -1 : 1;
        if (sel == 3) wb_act(dir);
        else if (sel == 2) gain_act(dir);
        else cxobj_panel_step(sel, dir);
    }
    force = 1;
    refresh();
    return 1;
}
