/*
 * Menú de fábrica: muestra u oculta ítems sin tocar el firmware.
 *
 * Detalle del análisis en docs/research/menu-fabrica.md. Cada menú es un árbol
 * estático de CmnViewMenuItemStruct (0x28 bytes) en .data de viewUnified7.so:
 * raíz -> pestañas -> ítems. Aparte hay un arreglo de 3 bytes por id
 * {opción elegida, estado, válido}; un ítem se ve si estado > 1. En la cámara,
 * poner válido = 0 después de initMenuData no ocultó nada (v18, 2026-11-27); los
 * ítems que oculta el firmware llegan con estado 0 y válido 1, y eso se imita.
 * CmnViewMenuData::initMenuData (libObj.so, llamada por la PLT desde la vista)
 * le pregunta el estado de cada ítem al «exclusivo» del menú. En la CX580V ése
 * oculta IRIS (12), Shutter Speed (13), AGC Limit (14), AE Shift (15) y White
 * Balance Shift (16) de Camera/Mic, que existen con sus pantallas.
 *
 * Aquí se intercepta initMenuData: se llama a la original y, antes de volver,
 * se corrige el estado de los ítems pedidos. Tiene que ser en ese momento,
 * porque justo después la vista cuenta las filas visibles para armar las páginas.
 *
 * cxmode.txt:  show <raíz> <id>    raíz: desplazamiento hexadecimal de la tabla
 *              hide <raíz> <id>    en viewUnified7.so 1.07 (c347c = Camera/Mic)
 */
#include "cx.h"

#ifndef CX_MENU_LIB
#define CX_MENU_LIB "/libObj.so"
#endif
#ifndef CX_MENU_VIEW_LIB
#define CX_MENU_VIEW_LIB "/viewUnified7.so"
#endif
#define SYM_INIT "_ZN15CmnViewMenuData12initMenuDataEP21CmnViewMenuItemStructP26CmnViewMenuItemInconstPartP22CmnViewMenuExclusiveIF"
#define NRULE 32

struct item {                 /* CmnViewMenuItemStruct */
    struct item *child;
    u8 n, pad[3];
    int id;
    u32 text, icon, help, value_icon;
    u8 row, exec, pad2[2];
    int opt_num;
    const char *opt_str;
};

/* Menús conocidos del firmware 1.07: raíz, id de la raíz (comprobación) e id máximo. */
static const struct { u32 root; int root_id, max_id; } menus[] = {
    { 0xc347c, 255, 254 },    /* view/CAMERA_SETTING_MENU (Camera/Mic) */
    { 0xb7220, 78, 77 },      /* view/IMAGE_QUALITY_SIZE_MENU */
    { 0xbb980, 206, 205 },    /* view/SETUP_MENU */
    { 0xc0380, 28, 27 },      /* view/TCUB_MENU */
};

static struct { u32 root; int id, show, logged; } rule[NRULE];
static int nrule;
static u32 r_init, view_base;
static int seen, base_tries;

void cxmenu_add(u32 root, u32 id, int show)
{
    unsigned k;
    if (nrule >= NRULE) return;
    for (k = 0; k < sizeof(menus) / sizeof(menus[0]); k++)
        if (menus[k].root == root && id >= 1 && (int)id <= menus[k].max_id) {
            rule[nrule].root = root;
            rule[nrule].id = (int)id;
            rule[nrule].show = show;
            nrule++;
            return;
        }
}

static int aligned(u32 p) { return p && !(p & 3); }

/* ¿Hay un ítem (nieto de la raíz) con ese id? */
static int has_item(const struct item *root, int id)
{
    int c, i;
    if (!aligned((u32)root->child) || root->n > 16) return 0;
    for (c = 0; c < root->n; c++) {
        const struct item *cat = &root->child[c];
        if (!aligned((u32)cat->child) || cat->n > 64) continue;
        for (i = 0; i < cat->n; i++)
            if (cat->child[i].id == id) return 1;
    }
    return 0;
}

u32 cxw_initmenu(u32 self, u32 root, u32 inc, u32 excl) __asm__(SYM_INIT);
u32 cxw_initmenu(u32 self, u32 root, u32 inc, u32 excl)
{
    struct line l;
    u32 r, off;
    unsigned k;
    int i;
    if (!r_init) r_init = cx_dlsym(cx_lib_base(CX_MENU_LIB), SYM_INIT);
    if (!r_init) return 0;
    r = ((u32 (*)(u32, u32, u32, u32))r_init)(self, root, inc, excl);
    if (!nrule || !aligned(root) || !inc) return r;
    if (!view_base && base_tries < 8) { base_tries++; view_base = cx_lib_base(CX_MENU_VIEW_LIB); }
    off = view_base && root >= view_base ? root - view_base : 0;
    if (seen < 6 && cx_log_enabled()) {      /* qué menús se abren, para la bitácora */
        seen++;
        cxl_begin(&l, "MN");
        cxl_lit(&l, " init menu=");
        cxl_hex(&l, off);
        cxl_lit(&l, " root id=");
        cxl_hex(&l, (u32)((const struct item *)root)->id);
        cxl_end(&l);
    }
    if (!off) return r;
    for (k = 0; k < sizeof(menus) / sizeof(menus[0]); k++)
        if (menus[k].root == off) break;
    if (k == sizeof(menus) / sizeof(menus[0])) return r;
    if (((const struct item *)root)->id != menus[k].root_id) return r;   /* no es el firmware esperado */
    for (i = 0; i < nrule; i++) {
        u8 *st;
        u8 was_state, was_valid;
        if (rule[i].root != off || !has_item((const struct item *)root, rule[i].id)) continue;
        st = (u8 *)inc + rule[i].id * 3;
        was_state = st[1];
        was_valid = st[2];
        if (rule[i].show) {
            if (st[1] < 2) st[1] = 2;
            st[2] = 1;
        } else {
            st[1] = 0;      /* igual que los ítems que oculta el firmware: estado 0, válido intacto */
        }
        if (rule[i].logged < 2 && cx_log_enabled()) {
            rule[i].logged++;
            cxl_begin(&l, "MN");
            cxl_lit(&l, rule[i].show ? " show menu=" : " hide menu=");
            cxl_hex(&l, off);
            cxl_lit(&l, " id=");
            cxl_hex(&l, (u32)rule[i].id);
            cxl_lit(&l, " state ");
            cxl_hex(&l, was_state);
            cxl_lit(&l, "->");
            cxl_hex(&l, st[1]);
            cxl_lit(&l, " valid ");
            cxl_hex(&l, was_valid);
            cxl_lit(&l, "->");
            cxl_hex(&l, st[2]);
            cxl_end(&l);
        }
    }
    return r;
}

void cxmenu_start(void)
{
    if (!r_init) r_init = cx_dlsym(cx_lib_base(CX_MENU_LIB), SYM_INIT);
}
