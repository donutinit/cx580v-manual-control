/*
 * Programa de prueba para QEMU. Sin libc.
 *
 *   cxtest        sólo espera (prueba el hilo de cxinh sin tabla)
 *   cxtest init   inicializa la tabla de bloqueos de la libMWF.so de la cámara
 *                 y marca bits conocidos
 *   cxtest view   llama por la PLT a las funciones de ViewBase de libcxt.so,
 *                 como hacen las pantallas, y comprueba que lleguen intactas
 *
 * Al final espera unos segundos para que los hilos de cxg.so escriban.
 */
#include "../cx.h"

void mwf_inh_initialize(void) __asm__("_ZN3MWF7InhInfo10InitializeEv");
/* Marca bits conocidos, como haría un InhMgr al activar un factor. */
void mwf_inh_or(u32 feature, u32 word, u32 value) __asm__("_ZN3MWF7InhInfo13OrInhTblValueEjjj");

u32 v_open(u32 self, const char *name, u32 n) __asm__("_ZN8ViewBase8openViewEPKci");
u32 v_close(u32 self, const char *name) __asm__("_ZN8ViewBase9closeViewEPKc");
u32 v_exec(u32 self, const char *model, u32 action, u32 plist) __asm__("_ZN8ViewBase19requestModelExecuteEPKcmP9ParamList");
u32 v_info(u32 self, const char *model, u32 id, u32 out, u32 in) __asm__("_ZN8ViewBase12getModelInfoEPKciP9ParamListPKS2_");
extern u32 cxt_args[5][6];
extern u32 cxt_info_value, cxt_exec_value;
extern u32 cxt_obj[2][4];
extern u32 cxt_seen[16][9];
extern u32 cxt_async_ret, cxt_sync_seen[4], cxt_check_feature, cxt_inh_blocked[4];
extern u32 cxt_text[2][3];
extern u32 cxt_image[4];
extern u32 cxt_textc[3], cxt_mouse[3][6], cxt_push[3];
extern u32 cxt_menu[5];
extern u32 cxt_lab[8][10], cxt_labfont[8][5], cxt_nlab, cxt_sibling, cxt_drawn, cxt_updres;
extern char cxt_labtext[8][20];
extern u32 cxt_tree[6], cxt_rootpath, cxt_rootnode[2];
extern u32 cxt_views[4];
u32 vb_ctor(u32 self, u32 mgr) __asm__("_ZN8ViewBaseC2EP11ViewManager");
u32 vb_dtor(u32 self) __asm__("_ZN8ViewBaseD2Ev");
u32 ge_updres(u32 self) __asm__("_ZN2ux6gfxeng14GraphicsEngine14updateResourceEv");
u32 md_init(u32 self, u32 root, unsigned char *inc, u32 excl)
    __asm__("_ZN15CmnViewMenuData12initMenuDataEP21CmnViewMenuItemStructP26CmnViewMenuItemInconstPartP22CmnViewMenuExclusiveIF");

/* Árbol de menú falso, como el de Camera/Mic: la raíz queda a 0xc347c de la base (Makefile). */
struct mitem { struct mitem *child; unsigned char n, pad[3]; int id; u32 text, icon, help, value_icon;
               unsigned char row, exec, pad2[2]; int opt_num; const char *opt_str; };
static struct mitem m_items[4] = {
    { 0, 0, {0}, 10, 0x02042614, 0, 0, 0, 10, 5, {0}, 0, "view/EXPOSURE" },
    { 0, 0, {0}, 12, 0x020428b6, 0, 0, 0, 10, 5, {0}, 0, "view/IRIS_L" },
    { 0, 0, {0}, 13, 0x02042b9c, 0, 0, 0, 10, 5, {0}, 0, "view/SHUTTER_SPEED_L" },
    { 0, 0, {0}, 17, 0x0204293a, 0, 0, 0, 10, 0, {0}, 0, 0 },
};
static struct mitem m_cats[1] = { { m_items, 4, {0}, 0, 0x02042da0, 0, 0, 0, 0, 0, {0}, 0, 0 } };
__attribute__((section(".cxmenu"), used))
static struct mitem m_root = { m_cats, 1, {0}, 255, 0, 0, 0, 0, 0, 0, {0}, 0, 0 };
static struct mitem m_other = { m_cats, 1, {0}, 255, 0, 0, 0, 0, 0, 0, {0}, 0, 0 };   /* misma forma, otro lugar */
static unsigned char m_inc[256 * 3];
u32 ws_move(u32 ws, u32 mouse, u32 x, u32 y, u32 t) __asm__("_ZN2ux6wgtsys12WidgetSystem13postMouseMoveEhssNS_4core8MsecTimeE");
u32 ws_press(u32 ws, u32 mouse, u32 button, u32 t) __asm__("_ZN2ux6wgtsys12WidgetSystem14postMousePressEhhNS_4core8MsecTimeE");
u32 ws_release(u32 ws, u32 mouse, u32 button, u32 t) __asm__("_ZN2ux6wgtsys12WidgetSystem16postMouseReleaseEhhNS_4core8MsecTimeE");
u32 em_push(u32 self, u32 ev) __asm__("_ZN12EventManager4pushEP5Event");
void gi_setimage(u32 self, u32 id, u32 flag) __asm__("_ZN8GEN_Icon8setImageEjb");
void ts_setstr(u32 self, u32 id) __asm__("_ZN2ux6gfxeng10TextSprite9setStringEj");
u32 rm_gettext(u32 self, u32 id, u32 out) __asm__("_ZN2ux6resmgr19ResourceManagerImpl7getTextEjRNS_4core7UStringE");
/* libMWF.so de la cámara, para armar notificaciones reales. */
void mwf_msg_ctor(u32 *m, u32 cat, u32 mid) __asm__("_ZN3MWF6ObjMsgC1Ejj");
void mwf_msg_dtor(u32 *m) __asm__("_ZN3MWF6ObjMsgD1Ev");
void mwf_us_ctor(u32 *p, unsigned short v) __asm__("_ZN3MWF17ParamStd_TemplateItLNS_5Param4TypeE6EEC1Et");
void mwf_us_dtor(u32 *p) __asm__("_ZN3MWF17ParamStd_TemplateItLNS_5Param4TypeE6EED1Ev");
void mwf_sc_ctor(u32 *p, signed char v) __asm__("_ZN3MWF17ParamStd_TemplateIaLNS_5Param4TypeE3EEC1Ea");
void mwf_sc_dtor(u32 *p) __asm__("_ZN3MWF17ParamStd_TemplateIaLNS_5Param4TypeE3EED1Ev");
void mwf_add(u32 *m, u32 id, u32 *p) __asm__("_ZN3MWF6ObjMsg8AddParamEjRKNS_5ParamE");
u32 mwf_get(u32 *m, u32 id) __asm__("_ZNK3MWF6ObjMsg8GetParamEj");
void mwf_msg_ctor0(u32 *m) __asm__("_ZN3MWF6ObjMsgC1Ev");
void mwf_getinh(u32 feature, u32 *m) __asm__("_ZN3MWF7InhInfo10GetInhInfoEN6DefInh11FeatureId_tEPNS_6ObjMsgE");

static void say(const char *s);
static int check(const char *what, int ok);
u32 o_async(u32 obj, u32 msg) __asm__("_ZN3MWF5ObjIf17IssueCommandAsyncEPvPNS_6ObjMsgE");
u32 o_sync(u32 obj, u32 in, u32 out) __asm__("_ZN3MWF5ObjIf16IssueCommandSyncEPvPNS_6ObjMsgES3_");


static void say(const char *s) { long n = 0; while (s[n]) n++; cx_write(1, s, n); }

/* ¿Alguno de los argumentos de /proc/self/cmdline es exactamente w? */
static int has_arg(const char *buf, long n, const char *w)
{
    long i, j;
    for (i = 0; i < n; i++) {
        if (i > 0 && buf[i - 1] != 0) continue;
        for (j = 0; w[j] && i + j < n && buf[i + j] == w[j]; j++) ;
        if (!w[j] && (i + j == n || buf[i + j] == 0)) return 1;
    }
    return 0;
}

static int check(const char *what, int ok)
{
    say(ok ? "cxtest: ok " : "cxtest: FALLA ");
    say(what);
    say("\n");
    return ok;
}

/* ParamList falsas con un nodo {vtable, tipo, clave, valor}. */
static u32 node[4] = { 0, 1, 0x1c4, 7 };
static u32 nodes[1], vec[3], refc = 1, plist[2];
static u32 onode[4] = { 0, 1, 0x12c, 0 };
static u32 onodes[1], ovec[3], oplist[2];

/* ObjMsg falso: msg[0] -> cuerpo {0, cat, mid, cuenta, bloque}; bloque {0, id, valor, tipo*}. */
static u32 t_int[1] = { 7 };
static u32 t_schar[1] = { 3 };   /* así llega EXPOSURE_BIAS en la cámara (p9652:3=...) */
static u32 o_chunk[16], o_body[8], o_msg[2];
static u32 q_chunk[16], q_body[8], q_msg[2];
static void mkmsg(u32 *msg, u32 *body, u32 *chunk, u32 cat, u32 mid, u32 id, u32 val)
{
    chunk[0] = 0; chunk[1] = id; chunk[2] = val; chunk[3] = (u32)t_int;
    body[0] = 0; body[1] = cat; body[2] = mid; body[3] = 1; body[4] = (u32)chunk;
    msg[0] = (u32)body; msg[1] = 0;
}

static void obj_tests(void)
{
    int ok = 1;
    u32 r;
    mkmsg(o_msg, o_body, o_chunk, 0x3100, 0x75b4, 0x96d3, 0x1cb);
    r = o_async(0x8888, (u32)o_msg);
    ok &= check("IssueCommandAsync: retorno y argumentos", r == 0xa5 && cxt_obj[0][0] == 0x8888 && cxt_obj[0][1] == (u32)o_msg);
    o_async(0x8888, (u32)o_msg);                 /* repetido: no se anota otra vez */
    o_chunk[2] = 0x1ca;
    o_async(0x8888, (u32)o_msg);                 /* cambió el valor: se anota */
    mkmsg(o_msg, o_body, o_chunk, 0x2000, 0x1234, 1, 2);
    o_async(0x8888, (u32)o_msg);                 /* otra categoría: no se anota */
    mkmsg(o_msg, o_body, o_chunk, 0x3100, 0x75d0, 0x951e, 0);
    mkmsg(q_msg, q_body, q_chunk, 0x3100, 0x75d0, 0x951e, 0);
    r = o_sync(0x9999, (u32)o_msg, (u32)q_msg);
    ok &= check("IssueCommandSync: retorno, argumentos y salida", r == 0x5c && cxt_obj[1][0] == 0x9999 &&
                cxt_obj[1][1] == (u32)o_msg && cxt_obj[1][2] == (u32)q_msg && q_chunk[2] == 0x1cb);
    ok &= check("IssueCommandAsync: 4 llamadas pasaron", cxt_obj[0][3] == 4);
    say(ok ? "cxtest: obj TODO OK\n" : "cxtest: obj FALLA\n");
}

/* cxmode.txt = "objmanual 1": EXP_MODE_IRIS/SHUTTER salen como MANUAL; AUTO_2 intacto. */
static void objmanual_tests(int expect)
{
    int ok = 1;
    mkmsg(o_msg, o_body, o_chunk, 0x3100, 0x7558, 0x95c8, 0xf3);
    o_async(0x8888, (u32)o_msg);
    ok &= check(expect ? "objmanual: IRIS -> MANUAL" : "sin objmanual: IRIS intacto", o_chunk[2] == (expect ? 0xf5u : 0xf3u));
    mkmsg(o_msg, o_body, o_chunk, 0x3100, 0x7558, 0x95c8, 0xf4);
    o_async(0x8888, (u32)o_msg);
    ok &= check(expect ? "objmanual: SHUTTER -> MANUAL" : "sin objmanual: SHUTTER intacto", o_chunk[2] == (expect ? 0xf5u : 0xf4u));
    mkmsg(o_msg, o_body, o_chunk, 0x3100, 0x7558, 0x95c8, 0xf7);
    o_async(0x8888, (u32)o_msg);
    ok &= check("objmanual: AUTO_2 intacto", o_chunk[2] == 0xf7);
    mkmsg(o_msg, o_body, o_chunk, 0x3100, 0x759e, 0x95c8, 0xf3);
    o_async(0x8888, (u32)o_msg);
    ok &= check("objmanual: otro mensaje intacto", o_chunk[2] == 0xf3);
    if (expect) {
        /* Con M activo, la pantalla de obturación abierta ve 3 aunque el modelo diga 6. */
        nodes[0] = (u32)node; vec[0] = (u32)nodes; vec[1] = (u32)(nodes + 1); vec[2] = vec[1];
        plist[0] = (u32)vec; plist[1] = (u32)&refc;
        onodes[0] = (u32)onode; ovec[0] = (u32)onodes; ovec[1] = (u32)(onodes + 1); ovec[2] = ovec[1];
        oplist[0] = (u32)ovec; oplist[1] = (u32)&refc;
        onode[1] = 1; onode[2] = 0x12b;
        mkmsg(o_msg, o_body, o_chunk, 0x3100, 0x7558, 0x95c8, 0xf4);
        o_async(0x8888, (u32)o_msg);                    /* M activo */
        v_open(0x6666, "view/SHUTTER_SPEED", 0);
        cxt_info_value = 6;
        v_info(0x6666, "model/CAMERA", 0xf, (u32)oplist, 0);
        ok &= check("objmanual: con M activo, Shutter Speed ve 3", onode[3] == 3);
        mkmsg(o_msg, o_body, o_chunk, 0x3100, 0x7558, 0x95c8, 0xf7);
        o_async(0x8888, (u32)o_msg);                    /* automático: M ya no está activo */
        v_info(0x6666, "model/CAMERA", 0xf, (u32)oplist, 0);
        ok &= check("objmanual: tras volver a automático, ve el real (6)", onode[3] == 6);
        v_close(0x6666, "view/SHUTTER_SPEED_L");
    }
    say(ok ? "cxtest: obj TODO OK\n" : "cxtest: obj FALLA\n");
}

/* ¿El mensaje asíncrono número k trae ese mensaje y ese parámetro con ese valor? */
static int seen_msg(u32 k, u32 mid, u32 pid, u32 val)
{
    u32 *e = cxt_seen[k & 15], i;
    if (e[0] != 0x3100 || e[1] != mid) return 0;
    for (i = 0; i < 3; i++)
        if (e[3 + 2 * i] == pid && e[4 + 2 * i] == val) return 1;
    return 0;
}

/* Índice (desde from, sin pasar de to) del primer mensaje con ese mensaje, parámetro y valor, o -1. */
static int find_seen(u32 from, u32 to, u32 mid, u32 pid, u32 val)
{
    u32 k;
    for (k = from; k < to; k++)
        if (seen_msg(k, mid, pid, val)) return (int)k;
    return -1;
}

/* Llamadas de la interfaz que dejan salir lo pendiente (soltar el AE lock, refijar). */
static void hticks(void)
{
    int i;
    for (i = 0; i < 3; i++) v_info(0x6666, "model/CAMERA", 0x13, (u32)oplist, 0);
}

static void open_iris_and_query(u32 model_says)
{
    v_open(0x6666, "view/EXPOSURE", 0);              /* -> view/IRIS */
    cxt_info_value = model_says;
    v_info(0x6666, "model/CAMERA", 0xf, (u32)oplist, 0);
}

/* cxmode.txt = "holdmanual 1\nredirect view/EXPOSURE view/IRIS". */
static void hold_tests(void)
{
    int ok = 1, a, b, c;
    u32 n;
    nodes[0] = (u32)node; vec[0] = (u32)nodes; vec[1] = (u32)(nodes + 1); vec[2] = vec[1];
    plist[0] = (u32)vec; plist[1] = (u32)&refc;
    onodes[0] = (u32)onode; ovec[0] = (u32)onodes; ovec[1] = (u32)(onodes + 1); ovec[2] = ovec[1];
    oplist[0] = (u32)ovec; oplist[1] = (u32)&refc;
    onode[1] = 1; onode[2] = 0x12b;
    cxt_async_ret = 0;   /* como la cámara: 0 si lo acepta */

    n = cxt_obj[0][3];
    mkmsg(o_msg, o_body, o_chunk, 0x3100, 0x7558, 0x95c8, 0xf4);   /* obturación manual */
    o_async(0x8888, (u32)o_msg);
    ok &= check("hold: el primer manual no inyecta nada", cxt_obj[0][3] == n + 1);

    n = cxt_obj[0][3];
    mkmsg(o_msg, o_body, o_chunk, 0x3100, 0x7558, 0x95c8, 0xf3);   /* iris manual */
    o_async(0x8888, (u32)o_msg);
    ok &= check("hold: el AE lock no se suelta pegado al cambio (la cámara rechaza el mismo mensaje en curso)",
                cxt_obj[0][3] == n + 3);
    hticks();
    {
        int lk = find_seen(n, cxt_obj[0][3], 0x7610, 0x9795, 0x2d6);
        int ul;
        b = find_seen(n, cxt_obj[0][3], 0x7558, 0x95c8, 0xf3);
        a = find_seen(n, cxt_obj[0][3], 0x75b6, 0x96d6, 0x1cd);
        ul = find_seen(n, cxt_obj[0][3], 0x7610, 0x9795, 0x2d8);
        c = ul >= 0 ? find_seen((u32)ul + 1, cxt_obj[0][3], 0x75b6, 0x96d6, 0x1cd) : -1;
        ok &= check("hold: obturación -> iris: AE lock, cambio de modo, obturación manual, AE unlock, obturación manual",
                    lk >= 0 && b > lk && a > b && ul > a && c > ul && cxt_obj[0][3] == n + 5);
        ok &= check("hold: el mensaje inyectado lleva el relevo (parámetro 0)", a >= 0 && cxt_seen[a & 15][2] == 2);
    }
    ok &= check("hold: va al mismo objeto", cxt_obj[0][0] == 0x8888);

    n = cxt_obj[0][3];
    mkmsg(o_msg, o_body, o_chunk, 0x3100, 0x7558, 0x95c8, 0xf4);   /* vuelta a obturación */
    o_async(0x8888, (u32)o_msg);
    hticks();
    a = find_seen(n, cxt_obj[0][3], 0x7610, 0x9795, 0x2d6);
    b = find_seen(n, cxt_obj[0][3], 0x7558, 0x95c8, 0xf4);
    c = find_seen(n, cxt_obj[0][3], 0x7610, 0x9795, 0x2d8);
    ok &= check("hold: iris -> obturación: AE lock antes de f4, unlock después, y el iris no se fija antes",
                a >= 0 && b > a && c > b && find_seen(n, (u32)b, 0x75b4, 0x96d3, 0x1cb) < 0 &&
                find_seen((u32)b, (u32)c, 0x75b4, 0x96d3, 0x1cb) > b && find_seen((u32)c, cxt_obj[0][3], 0x75b4, 0x96d3, 0x1cb) > c);

    n = cxt_obj[0][3];
    mkmsg(o_msg, o_body, o_chunk, 0x3100, 0x7558, 0x95c8, 0xf4);   /* la misma prioridad otra vez */
    o_async(0x8888, (u32)o_msg);
    ok &= check("hold: repetir la prioridad no bloquea, sólo reafirma después",
                cxt_obj[0][3] == n + 2 && find_seen(n + 1, n + 2, 0x75b4, 0x96d3, 0x1cb) == (int)(n + 1));

    /* La pantalla de iris, abierta con el modelo en 3, ve 2 (iris manual). */
    open_iris_and_query(3);
    ok &= check("hold: Iris abierta con iris manual ve 2 (el modelo dice 3)", onode[3] == 2);

    /* Primer toque: el paso se vuelve a=10 modo 2 y la lista queda así. */
    node[1] = 1; node[2] = 0x1a7; node[3] = 1;
    v_exec(0x6666, "model/CAMERA", 0x2d, (u32)plist);
    ok &= check("hold: primer toque de iris llega como a=10 modo 2 (y la lista queda así)",
                cxt_args[2][2] == 0x10 && cxt_exec_value == 2 && node[2] == 0x12a && node[3] == 2);
    node[2] = 0x1a7; node[3] = 1;
    v_exec(0x6666, "model/CAMERA", 0x2d, (u32)plist);
    ok &= check("hold: el segundo toque pasa como paso de iris", cxt_args[2][2] == 0x2d && cxt_exec_value == 1);

    /* Automático en Iris con la obturación manual: modo 3 en el modelo e IRIS_AUTO en la cámara. */
    n = cxt_obj[0][3];
    node[2] = 0x12a; node[3] = 5;
    v_exec(0x6666, "model/CAMERA", 0x10, (u32)plist);
    ok &= check("hold: automático en Iris con obturación manual manda prioridad de obturación (3)", cxt_exec_value == 3);
    ok &= check("hold: y pone el iris en automático en la cámara (IRIS_AUTO)",
                find_seen(n, cxt_obj[0][3], 0x75b4, 0x96d3, 0x1ca) >= 0);
    v_close(0x6666, "view/IRIS_L");
    n = cxt_obj[0][3];
    mkmsg(o_msg, o_body, o_chunk, 0x3100, 0x7558, 0x95c8, 0xf4);
    o_async(0x8888, (u32)o_msg);
    ok &= check("hold: tras el automático del iris ya no se fija el iris", find_seen(n, cxt_obj[0][3], 0x75b4, 0x96d3, 0x1cb) < 0);

    n = cxt_obj[0][3];
    mkmsg(o_msg, o_body, o_chunk, 0x3100, 0x7558, 0x95c8, 0xf7);   /* automático */
    o_async(0x8888, (u32)o_msg);
    ok &= check("hold: al automático no se inyecta nada", cxt_obj[0][3] == n + 1);
    open_iris_and_query(6);
    ok &= check("hold: tras el automático la pantalla ve 6", onode[3] == 6);
    v_close(0x6666, "view/IRIS_L");
    cxt_async_ret = 0xa5;
    say(ok ? "cxtest: hold TODO OK\n" : "cxtest: hold FALLA\n");
}

/* Mensaje de AE Shift falso: dos parámetros {SW, BIAS}. */
static void mkbias(u32 sw, u32 bias)
{
    mkmsg(o_msg, o_body, o_chunk, 0x3100, 0x759e, 0x9654, sw);
    o_chunk[4] = 0x9652; o_chunk[5] = bias; o_chunk[6] = (u32)t_int;
    o_body[3] = 2;
}

/* cxmode.txt = "gainshift 1". */
static void gain_tests(void)
{
    int ok = 1;
    u32 n0 = cxt_obj[0][3], *e;
    mkbias(0x199, 2);                                   /* AE Shift +2 */
    o_async(0x8888, (u32)o_msg);
    ok &= check("gain: el AE Shift real pasa intacto (SW ON, bias 2)", o_chunk[2] == 0x199 && o_chunk[5] == 2);
    e = cxt_seen[(n0 + 1) & 15];
    ok &= check("gain: +2 inyecta SET_AGC_LIMIT", cxt_obj[0][3] == n0 + 2 && e[1] == 0x75ba);
    ok &= check("gain: AGC_LIMIT = 18 dB", seen_msg(n0 + 1, 0x75ba, 0x96dc, 18));
    ok &= check("gain: AGC_LIMIT_MODE = ON", seen_msg(n0 + 1, 0x75ba, 0x96dd, 0x1d0));
    mkbias(0x199, (u32)-4);                             /* AE Shift -4 */
    o_async(0x8888, (u32)o_msg);
    ok &= check("gain: -4 pone 0 dB", seen_msg(n0 + 3, 0x75ba, 0x96dc, 0));
    mkbias(0x19a, 0);                                   /* AE Shift apagado */
    o_async(0x8888, (u32)o_msg);
    ok &= check("gain: AE Shift apagado apaga el límite", seen_msg(n0 + 5, 0x75ba, 0x96dd, 0x1d1));
    say(ok ? "cxtest: gain TODO OK\n" : "cxtest: gain FALLA\n");
}


/* Notificación como la de la cámara, leída con GetParam como ObjCamera. */
static u32 notify(u32 mid, u32 id1, u32 v1, u32 id2, u32 v2)
{
    u32 m[4] __attribute__((aligned(8)));
    u32 p[4] __attribute__((aligned(8)));
    u32 got = 0, q, i;
    for (i = 0; i < 4; i++) m[i] = p[i] = 0;
    mwf_msg_ctor(m, 0x3100, mid);
    mwf_us_ctor(p, (unsigned short)v1); mwf_add(m, id1, p); mwf_us_dtor(p);
    if (id2) { mwf_us_ctor(p, (unsigned short)v2); mwf_add(m, id2, p); mwf_us_dtor(p); }
    q = mwf_get(m, id1);
    if (q) got = *(u32 *)q & 0xffff;
    if (id2) mwf_get(m, id2);
    mwf_msg_dtor(m);
    return got;
}
static void fno(u32 f) { notify(0x766f, 0x951b, f, 0x9787, 18); }
static void fno_zoom(u32 f, u32 mn) { notify(0x766f, 0x951b, f, 0x9787, mn); }
static void shut(u32 n, u32 d) { notify(0x7670, 0x951c, n, 0x951d, d); }
static void setmode(u32 v)
{
    mkmsg(o_msg, o_body, o_chunk, 0x3100, 0x7558, 0x95c8, v);
    o_async(0x8888, (u32)o_msg);
}
/* Varias llamadas de la interfaz, con pausa, como las consultas de las pantallas. */
static void ticks(int n)
{
    int i;
    for (i = 0; i < n; i++) {
        v_info(0x6666, "model/CAMERA", 0x13, (u32)oplist, 0);
        cx_sleep_ms(50);
    }
}

/* cxmode.txt = "holdmanual 1". */
static void fix_tests(void)
{
    int ok = 1;
    u32 n;
    onodes[0] = (u32)onode; ovec[0] = (u32)onodes; ovec[1] = (u32)(onodes + 1); ovec[2] = ovec[1];
    oplist[0] = (u32)ovec; oplist[1] = (u32)&refc;
    cxt_async_ret = 0;

    ok &= check("fix: GetParam por la PLT devuelve el valor real (F2.8 = 28)", notify(0x766f, 0x951b, 28, 0, 0) == 28);

    setmode(0xf4);                     /* obturación manual */
    shut(1, 180);                      /* el usuario la deja en 1/180 */
    setmode(0xf3);                     /* iris manual */
    ticks(3);                          /* suelta el AE lock y vuelve a fijar la obturación */
    fno(28);                           /* el usuario deja F2.8 */
    n = cxt_obj[1][3];
    ticks(2);
    ok &= check("fix: sin cambio de prioridad no hay pasos", cxt_obj[1][3] == n);

    setmode(0xf4);                     /* vuelve a obturación */
    shut(1, 125);                      /* la cámara la pone en 1/125 */
    ticks(4);
    ok &= check("fix: obturación vuelve en 1/125 con objetivo 1/180: un paso +1",
                cxt_obj[1][3] == n + 1 && cxt_sync_seen[0] == 0x6504 && cxt_sync_seen[1] == 0x9574 &&
                (cxt_sync_seen[2] & 0xff) == 1);
    shut(1, 180);
    ticks(3);
    ok &= check("fix: en 1/180 se detiene", cxt_obj[1][3] == n + 1);

    /* Iris con el signo al revés: +1 abre en vez de cerrar. */
    setmode(0xf3);
    fno(24);                           /* la cámara lo pone en F2.4 */
    n = cxt_obj[1][3];
    ticks(3);
    ok &= check("fix: iris vuelve en F2.4 con objetivo F2.8: paso +1",
                cxt_obj[1][3] == n + 1 && cxt_sync_seen[0] == 0x6502 && (cxt_sync_seen[2] & 0xff) == 1);
    fno(22);                           /* se abrió: el signo estaba al revés */
    ticks(3);
    ok &= check("fix: aprende el signo y manda -1",
                cxt_obj[1][3] == n + 2 && (cxt_sync_seen[2] & 0xff) == 0xff);
    fno(24);
    ticks(3);
    ok &= check("fix: sigue con -1", cxt_obj[1][3] == n + 3 && (cxt_sync_seen[2] & 0xff) == 0xff);
    fno(28);
    ticks(3);
    ok &= check("fix: en F2.8 se detiene", cxt_obj[1][3] == n + 3);

    /* Un paso del usuario cancela la corrección. */
    setmode(0xf4);
    shut(1, 125);
    n = cxt_obj[1][3];
    {
        u32 m[4] __attribute__((aligned(8)));
        m[0] = m[1] = m[2] = m[3] = 0;
        mkmsg(q_msg, q_body, q_chunk, 0x3100, 0x6504, 0x9574, 1);
        o_sync(0x8888, (u32)q_msg, (u32)m);
    }
    ticks(4);
    ok &= check("fix: con un paso del usuario no se corrige", cxt_obj[1][3] == n + 1);

    /* El otro control: al entrar a iris, la obturación (1/125, objetivo 1/180) también se corrige. */
    setmode(0xf3);
    n = cxt_obj[1][3];
    ticks(4);
    ok &= check("fix: al entrar a iris se corrige también la obturación (paso +1)",
                cxt_obj[1][3] == n + 1 && cxt_sync_seen[0] == 0x6504 && (cxt_sync_seen[2] & 0xff) == 1);
    shut(1, 180);
    ticks(3);
    ok &= check("fix: la obturación queda en 1/180", cxt_obj[1][3] == n + 1);

    /* Desvío tardío: el valor llega bien y la cámara lo mueve unas décimas después. */
    setmode(0xf4);
    n = cxt_obj[1][3];
    ticks(3);                          /* todo en su valor: no hay pasos, pero sigue atento */
    ok &= check("fix: con todo en su valor no hay pasos", cxt_obj[1][3] == n);
    fno(24);                           /* el iris (objetivo F2.8) se mueve tarde, ya en prioridad de obturación */
    ticks(3);
    ok &= check("fix: un desvío que llega tarde también se corrige",
                cxt_obj[1][3] == n + 1 && cxt_sync_seen[0] == 0x6502);
    fno(28);
    ticks(2);

    /* Zoom: el F mínimo cambió, no se corrige el iris. */
    setmode(0xf3);
    ticks(2);
    setmode(0xf4);                     /* sale a obturación: el iris (F2.8) no se movió */
    ticks(2);
    fno_zoom(34, 30);                  /* zoom en prioridad de obturación */
    setmode(0xf3);                     /* iris otra vez; su objetivo sigue en F2.8 con mínimo 18 */
    n = cxt_obj[1][3];
    ticks(4);
    ok &= check("fix: si cambió el zoom no corrige el iris", cxt_obj[1][3] == n);

    /* Automático: ya no hay valor que restaurar. */
    setmode(0xf7);
    setmode(0xf3);
    fno(40);
    n = cxt_obj[1][3];
    ticks(3);
    ok &= check("fix: tras el automático no hay corrección", cxt_obj[1][3] == n);

    cxt_async_ret = 0xa5;
    say(ok ? "cxtest: fix TODO OK\n" : "cxtest: fix FALLA\n");
}

/* cxmode.txt = "isoshift 1". */
static void iso_tests(void)
{
    int ok = 1;
    u32 n0 = cxt_obj[0][3];
    mkbias(0x199, 2);                                   /* AE Shift +2 */
    o_async(0x8888, (u32)o_msg);
    ok &= check("iso: el AE Shift real pasa intacto", o_chunk[2] == 0x199 && o_chunk[5] == 2);
    ok &= check("iso: +2 inyecta SET_ISO_SPEED_RATE", cxt_obj[0][3] == n0 + 2 && cxt_seen[(n0 + 1) & 15][1] == 0x75d0);
    ok &= check("iso: +2 es ISO 1600 (0x38), mínimo igual",
                seen_msg(n0 + 1, 0x75d0, 0x951e, 0x38) && seen_msg(n0 + 1, 0x75d0, 0x96fe, 0x38));
    mkbias(0x199, (u32)-4);
    o_async(0x8888, (u32)o_msg);
    ok &= check("iso: -4 manda ISO 100", seen_msg(n0 + 3, 0x75d0, 0x951e, 0x2c));
    mkbias(0x19a, 0);
    o_async(0x8888, (u32)o_msg);
    ok &= check("iso: AE Shift apagado manda ISO automático de 100 a 6400",
                seen_msg(n0 + 5, 0x75d0, 0x951e, 0x21) && seen_msg(n0 + 5, 0x75d0, 0x96fe, 0x2c));
    ok &= check("iso: no toca el límite de ganancia", cxt_obj[0][3] == n0 + 6);
    say(ok ? "cxtest: iso TODO OK\n" : "cxtest: iso FALLA\n");
}

/* cxmode.txt = "expogain 1". */
static void expo_tests(void)
{
    int ok = 1;
    u32 n0 = cxt_obj[0][3], s0 = cxt_obj[1][3];
    mkbias(0x199, 0);                                   /* AE Shift encendido en 0 */
    o_async(0x8888, (u32)o_msg);
    ok &= check("expo: encender AE Shift enciende la exposición manual (0x95bd = 0xea)",
                cxt_obj[0][3] == n0 + 2 && seen_msg(n0 + 1, 0x7552, 0x95bd, 0xea) && cxt_obj[1][3] == s0);
    mkbias(0x199, 2);                                   /* +2: dos pasos */
    o_async(0x8888, (u32)o_msg);
    ok &= check("expo: +2 da dos pasos SYNC_MANUAL_EXPOSURE_DRIVE +1",
                cxt_obj[1][3] == s0 + 2 && cxt_sync_seen[0] == 0x6506 && cxt_sync_seen[1] == 0x9575 &&
                (cxt_sync_seen[2] & 0xff) == 1);
    mkbias(0x199, 1);
    o_async(0x8888, (u32)o_msg);
    ok &= check("expo: +1 da un paso -1", cxt_obj[1][3] == s0 + 3 && (cxt_sync_seen[2] & 0xff) == 0xff);
    mkbias(0x19a, 0);                                   /* apagado */
    o_async(0x8888, (u32)o_msg);
    ok &= check("expo: apagar AE Shift apaga la exposición manual",
                seen_msg(cxt_obj[0][3] - 1, 0x7552, 0x95bd, 0xeb) && cxt_obj[1][3] == s0 + 3);
    say(ok ? "cxtest: expo TODO OK\n" : "cxtest: expo FALLA\n");
}

/* ¿La función está bloqueada según GetInhInfo (por la PLT, como InhFeatureCheck)? */
static int inh_flag(u32 feature)
{
    u32 m[4] __attribute__((aligned(8)));
    u32 q, v = 2, i;
    for (i = 0; i < 4; i++) m[i] = 0;
    mwf_msg_ctor0(m);
    mwf_getinh(feature, m);
    q = mwf_get(m, 0x1000);
    if (q) v = *(u32 *)q & 0xff;
    mwf_msg_dtor(m);
    return (int)v;
}

/* cxtest unlock: GetInhInfo de libcxt.so dice bloqueadas 0x116 (ISO) y 0x174. */
static void unlock_tests(int expect_unlocked)
{
    int ok = 1;
    ok &= check("unlock: 0x174 sigue bloqueada", inh_flag(0x174) == 1);
    if (expect_unlocked)
        ok &= check("unlock: con unlockiso, 0x116 (ISO) se contesta libre", inh_flag(0x116) == 0);
    else
        ok &= check("unlock: sin unlockiso, 0x116 (ISO) sigue bloqueada", inh_flag(0x116) == 1);
    ok &= check("unlock: una función libre sigue libre", inh_flag(0x2d3) == 0);
    say(ok ? "cxtest: unlock TODO OK\n" : "cxtest: unlock FALLA\n");
}

/* cxmode.txt = "expogain 1\nautounlock 1": el paso de exposición sale bloqueado por 0x174. */
static void auto_tests(void)
{
    int ok = 1;
    u32 s0;
    mkbias(0x199, 0);
    o_async(0x8888, (u32)o_msg);                        /* enciende la exposición manual */
    cxt_check_feature = 0x174;
    s0 = cxt_obj[1][3];
    mkbias(0x199, 1);
    cxt_check_feature = 0;                              /* el mensaje del usuario no se revisa */
    {
        /* Sólo la orden propia (el paso) pasa por la revisión. */
        u32 before = cxt_obj[1][3];
        cxt_check_feature = 0x174;
        mkbias(0x199, 1);
        o_async(0x8888, (u32)o_msg);
        ok &= check("auto: el paso bloqueado se reintenta libre y llega", cxt_obj[1][3] == before + 1 && cxt_sync_seen[0] == 0x6506);
    }
    cxt_check_feature = 0x116;
    ok &= check("auto: otra función bloqueada (0x116) no se libera sola", inh_flag(0x116) == 1);
    ok &= check("auto: la aprendida (0x174) se contesta libre", inh_flag(0x174) == 0);
    cxt_check_feature = 0;
    (void)s0;
    say(ok ? "cxtest: auto TODO OK\n" : "cxtest: auto FALLA\n");
}

/* La pantalla lee el AE Shift que reporta la cámara (signed char), como pvt_ProcessExposureBiasCmpEvt. */
static int read_bias(u32 id, signed char camera_value)
{
    u32 m[4] __attribute__((aligned(8)));
    u32 p[4] __attribute__((aligned(8)));
    u32 q, i;
    int v = 99;
    for (i = 0; i < 4; i++) m[i] = p[i] = 0;
    mwf_msg_ctor(m, 0x3100, 0x76aa);
    mwf_sc_ctor(p, camera_value); mwf_add(m, id, p); mwf_sc_dtor(p);
    q = mwf_get(m, id);
    if (q) v = (signed char)(*(u32 *)q & 0xff);
    mwf_msg_dtor(m);
    return v;
}

/* cxmode.txt = "gainshift 1\ngainlock 4". */
static void lock_tests(void)
{
    int ok = 1;
    u32 n0 = cxt_obj[0][3];
    mkbias(0x199, 2);                                   /* la pantalla pide +2 */
    o_async(0x8888, (u32)o_msg);
    ok &= check("lock: a la cámara le llega +4", o_chunk[2] == 0x199 && o_chunk[5] == 4);
    ok &= check("lock: el tope de ganancia es el de +2 (18 dB)", seen_msg(n0 + 1, 0x75ba, 0x96dc, 18));
    ok &= check("lock: la pantalla ve +2 en EXPOSURE_BIAS_SET aunque la cámara diga +4", read_bias(0x9655, 4) == 2);
    ok &= check("lock: y en EXPOSURE_BIAS_ACT", read_bias(0x9658, 4) == 2);
    ok &= check("lock: otros parámetros pasan intactos", read_bias(0x9652, 4) == 4);
    mkbias(0x199, (u32)-3);
    o_async(0x8888, (u32)o_msg);
    ok &= check("lock: -3 también va como +4 y la pantalla ve -3", o_chunk[5] == 4 && read_bias(0x9655, 4) == -3);
    mkbias(0x19a, 0);                                   /* apagado */
    o_async(0x8888, (u32)o_msg);
    ok &= check("lock: apagado pasa intacto y la pantalla ve lo de la cámara", o_chunk[5] == 0 && read_bias(0x9655, 0) == 0);
    say(ok ? "cxtest: lock TODO OK\n" : "cxtest: lock FALLA\n");
}

/* cxmode.txt = "label 2614 28b6\nlabel 2bf1 26f0\nlabel zz 1\nlabel 12345 1\nimage 24 18f". */
static void text_tests(int expect)
{
    int ok = 1;
    u32 r;
    ts_setstr(0x7001, 0x02042614);                      /* Exposure */
    ok &= check(expect ? "text: Exposure (2614) se pide como IRIS (28b6), por la función Thumb"
                       : "text: sin label, Exposure pasa intacto",
                cxt_text[0][0] == 0x7001 && cxt_text[0][1] == (expect ? 0x020428b6u : 0x02042614u) && cxt_text[0][2] == 1);
    ts_setstr(0x7002, 0x02042bf1);                      /* Spot Meter/Fcs */
    ok &= check(expect ? "text: Spot Meter/Fcs (2bf1) se pide como Gain (26f0)" : "text: sin label, 2bf1 intacto",
                cxt_text[0][1] == (expect ? 0x020426f0u : 0x02042bf1u));
    ts_setstr(0x7003, 0x02042690);                      /* Focus: sin regla */
    ok &= check("text: un texto sin regla pasa intacto", cxt_text[0][0] == 0x7003 && cxt_text[0][1] == 0x02042690);
    ts_setstr(0x7004, 0x01002614);                      /* otro espacio de nombres */
    ok &= check("text: un número que no es de texto (no 0x0204xxxx) pasa intacto", cxt_text[0][1] == 0x01002614);
    ts_setstr(0x7005, 0x1e042614);                      /* con bits de tipo arriba */
    ok &= check("text: los bits altos del número se conservan",
                cxt_text[0][1] == (expect ? 0x1e0428b6u : 0x1e042614u));
    r = rm_gettext(0x7006, 0x02042614, 0x100);
    ok &= check(expect ? "text: getText también cambia el número y devuelve lo que devuelve la real"
                       : "text: sin label, getText intacto",
                cxt_text[1][0] == 0x7006 && cxt_text[1][1] == (expect ? 0x020428b6u : 0x02042614u) && r == 0x101);
    gi_setimage(0x7010, 0x02200024, 0);                 /* icono de AE Shift 0 */
    ok &= check(expect ? "image: el icono de AE Shift 0 (24) se dibuja como 12dB (18f)" : "image: sin regla, el icono pasa intacto",
                cxt_image[0] == 0x7010 && cxt_image[1] == (expect ? 0x0220018fu : 0x02200024u) && cxt_image[2] == 0);
    gi_setimage(0x7011, 0x02200026, 1);
    ok &= check("image: un icono sin regla pasa intacto, con su bandera", cxt_image[1] == 0x02200026 && cxt_image[2] == 1);
    gi_setimage(0x7012, 0x02042614, 0);
    ok &= check("image: las reglas de texto no tocan los iconos", cxt_image[1] == 0x02042614);
    ts_setstr(0x7007, 0x02040001);                      /* reglas mal escritas no cuentan */
    ok &= check("text: las reglas inválidas del archivo se ignoran", cxt_text[0][1] == 0x02040001);
    say(ok ? "cxtest: text TODO OK\n" : "cxtest: text FALLA\n");
}

/* cxmode.txt = "uxlog 1\ntext 2690 F2.8_1/60\n" (o vacío con expect = 0). */
static void ux_tests(int expect)
{
    int ok = 1;
    static u32 label[0x290 / 4];          /* GEN_Label falso; el sprite está en +0x144 */
    static u32 ev[4] = { 0, 0x15020041, 4, 0 };
    u32 r, i, sprite = (u32)label + 0x144;
    for (i = 0; i < 6; i++) label[(0x264 / 4) + i] = 0xf0f0f000u + i;
    r = ws_move(0x9001, 0, 0x0123, (u32)(short)-5 & 0xffff, 0xabcdef01);
    ok &= check("ux: postMouseMove llega igual, con su quinto argumento de la pila y su retorno",
                cxt_mouse[0][0] == 0x9001 && cxt_mouse[0][1] == 0 && cxt_mouse[0][2] == 0x0123 &&
                cxt_mouse[0][3] == 0xfffb && cxt_mouse[0][4] == 0xabcdef01 && cxt_mouse[0][5] == 1 && r == 0x111);
    r = ws_press(0x9001, 0, 0, 0x7700);
    ok &= check("ux: postMousePress (Thumb) llega igual", cxt_mouse[1][0] == 0x9001 && cxt_mouse[1][3] == 0x7700 &&
                cxt_mouse[1][5] == 1 && r == 0x222);
    r = ws_release(0x9001, 0, 0, 0x7701);
    ok &= check("ux: postMouseRelease llega igual", cxt_mouse[2][3] == 0x7701 && cxt_mouse[2][5] == 1 && r == 0x333);
    r = em_push(0x9002, (u32)ev);
    ok &= check("ux: EventManager::push llega igual", cxt_push[0] == 0x9002 && cxt_push[1] == (u32)ev && cxt_push[2] == 1 && r == 0);
    ev[1] = 0x00120034;                                  /* evento que no es de botón */
    em_push(0x9002, (u32)ev);
    em_push(0x9002, 0);                                  /* puntero nulo: tampoco debe fallar */
    ok &= check("ux: los demás eventos pasan sin anotarse", cxt_push[2] == 3);
    ts_setstr(sprite, 0x02042614);                       /* etiqueta de fábrica: anota la fuente */
    ok &= check("ux: la etiqueta pasa intacta mientras se anota su fuente", cxt_text[0][0] == sprite && cxt_text[0][1] == 0x02042614);
    ts_setstr(sprite, 0x02042690);                       /* con regla text: texto libre */
    if (expect)
        ok &= check("ux: text manda el texto libre por setString(char const*), con '_' como espacio",
                    cxt_textc[0] == sprite && cxt_textc[1] == 0x382e3246u /* "F2.8" */ && cxt_textc[2] == 1 &&
                    cxt_text[0][1] == 0x02042614);
    else
        ok &= check("ux: sin regla text la etiqueta pasa por setString(id)", cxt_textc[2] == 0 && cxt_text[0][1] == 0x02042690);
    ws_move(0x9001, 0, 10, 20, 0);                       /* un toque más: vuelca los botones anotados */
    say(ok ? "cxtest: ux TODO OK\n" : "cxtest: ux FALLA\n");
}

static int lab_is(int i, const char *t)
{
    int k = 0;
    while (t[k] && cxt_labtext[i][k] == t[k]) k++;
    return !t[k] && !cxt_labtext[i][k];
}

/* Un dedo: llega, aprieta, suelta y se va fuera de la pantalla. */
static void tap(u32 x, u32 y)
{
    ws_move(0x9001, 0, x, y, 0);
    ws_press(0x9001, 0, 0, 0);
    ws_release(0x9001, 0, 0, 0);
    ws_move(0x9001, 0, 2000, 2000, 0);
}

/*
 * Árbol de widgets falso para la capa del panel: raíz -> vista -> {botón W en
 * (590,150) 50x80 con un icono dentro, indicador en (130,440) 40x24, medidor de
 * audio en (440,400) 110x60 que no debe tocarse}. Campos como en la cámara.
 */
static u32 f_root[0x290 / 4], f_view[0x290 / 4], f_btn[0x290 / 4], f_icon[0x290 / 4], f_ind[0x290 / 4], f_meter[0x290 / 4];
static void fake(u32 *w, int x, int y, int ww, int hh, u32 id)
{
    w[0] = 0x12340000u;                  /* «vtable» */
    w[0x3c / 4] = (u32)(x << 16); w[0x40 / 4] = (u32)(y << 16);
    w[0x78 / 4] = (u32)(ww << 16); w[0x7c / 4] = (u32)(hh << 16);
    w[0x104 / 4] = id;
}
static int fmode(const u32 *w) { return ((const unsigned char *)w)[0x19]; }

static void panel_tree(void)
{
    fake(f_root, 0, 0, 640, 480, 0x10001);
    fake(f_view, 0, 0, 640, 480, 0x10002);
    fake(f_btn, 590, 150, 50, 80, 0x1b0001);
    fake(f_icon, 5, 5, 40, 70, 0x1c0001);      /* dentro del botón: posición relativa */
    fake(f_ind, 130, 440, 40, 24, 0x1d0001);
    fake(f_meter, 440, 400, 110, 60, 0x1e0001);
    cxt_tree[0] = (u32)f_root; cxt_tree[1] = (u32)f_view; cxt_tree[2] = (u32)f_btn; cxt_tree[3] = (u32)f_icon;
    cxt_tree[4] = (u32)f_ind; cxt_tree[5] = (u32)f_meter;
    cxt_rootnode[0] = (u32)f_root;
}

static void moves(int n)
{
    int i;
    for (i = 0; i < n; i++) ws_move(0x9001, 0, 2000, 2000, 0);
}

/* cxmode.txt = "panel 2\npanelwb 1": séptima lectura, balance de blancos con One Push. */
static void panelwb_tests(void)
{
    int ok = 1;
    u32 a0;
    cxt_rootpath = 1;
    panel_tree();
    v_open(0x5555, "view/MOV_REC", 0);
    setmode(0xf7);
    ok &= check("wb: séptima etiqueta abajo a la izquierda, en automático",
                cxt_nlab == 7 && lab_is(6, "AWB") && lab_is(5, "AUTO") && cxt_lab[6][4] == (4u << 16) && cxt_lab[6][5] == (418u << 16));
    tap(50, 440);
    ok &= check("wb: se elige tocándola", lab_is(6, "[AWB]") && lab_is(1, "F--A"));
    a0 = cxt_obj[0][3];
    tap(600, 150);                       /* + */
    ok &= check("wb: + pasa a sol", find_seen(a0, cxt_obj[0][3], 0x7546, 0x958d, 0xc8) >= 0 && lab_is(6, "[SOL]"));
    a0 = cxt_obj[0][3];
    tap(600, 240);                       /* AUTO */
    ok &= check("wb: AUTO vuelve al automático", find_seen(a0, cxt_obj[0][3], 0x7546, 0x958d, 0xc7) >= 0 && lab_is(6, "[AWB]"));
    a0 = cxt_obj[0][3];
    tap(600, 240);
    ok &= check("wb: AUTO otra vez vuelve al último preajuste", find_seen(a0, cxt_obj[0][3], 0x7546, 0x958d, 0xc8) >= 0 && lab_is(6, "[SOL]"));
    a0 = cxt_obj[0][3];
    tap(600, 150);
    ok &= check("wb: + pasa a interior", find_seen(a0, cxt_obj[0][3], 0x7546, 0x958d, 0xcf) >= 0 && lab_is(6, "[INT]"));
    a0 = cxt_obj[0][3];
    tap(600, 150);
    ok &= check("wb: + pasa a One Push y el botón dice SET",
                find_seen(a0, cxt_obj[0][3], 0x7546, 0x958d, 0xc3) >= 0 && lab_is(6, "[1P]") && lab_is(5, "SET"));
    a0 = cxt_obj[0][3];
    tap(600, 240);                       /* SET: fija el blanco con la carta gris */
    ok &= check("wb: SET manda las dos órdenes de la pantalla de fábrica y avisa",
                find_seen(a0, cxt_obj[0][3], 0x7542, 0x9586, 0) >= 0 && find_seen(a0, cxt_obj[0][3], 0x7542, 0x9587, 0) >= 0 &&
                find_seen(a0, cxt_obj[0][3], 0x7544, 0x958c, 0xc3) >= 0 && find_seen(a0, cxt_obj[0][3], 0x7546, 0x958d, 0xc3) < 0 &&
                lab_is(6, "[OK]"));
    a0 = cxt_obj[0][3];
    tap(600, 150);
    ok &= check("wb: + da la vuelta al automático y el botón vuelve a decir AUTO",
                find_seen(a0, cxt_obj[0][3], 0x7546, 0x958d, 0xc7) >= 0 && lab_is(6, "[AWB]") && lab_is(5, "AUTO"));
    tap(600, 330);                       /* - */
    ok &= check("wb: - regresa a One Push", lab_is(6, "[1P]"));
    mkmsg(o_msg, o_body, o_chunk, 0x3100, 0x7546, 0x958d, 0xcf);
    o_async(0x8888, (u32)o_msg);         /* la interfaz pone Indoor desde su menú */
    ws_move(0x9001, 0, 2000, 2000, 0);
    ok &= check("wb: muestra el preajuste que pone la interfaz", lab_is(6, "[INT]"));
    tap(150, 440);                       /* elige el iris */
    ok &= check("wb: con otro valor elegido el botón dice AUTO", lab_is(5, "AUTO") && lab_is(6, "INT"));
    say(ok ? "cxtest: wb TODO OK\n" : "cxtest: wb FALLA\n");
}

/* cxmode.txt = "panel 2\nkeepbox 450 410 90 40": de la pantalla sólo queda lo que toca esa caja. */
static void panelkeep_tests(void)
{
    int ok = 1;
    cxt_rootpath = 1;
    panel_tree();
    v_open(0x5555, "view/MOV_REC", 0);
    ok &= check("panel: con keepbox se oculta todo lo que no toca la caja; el medidor y la vista quedan",
                cxt_nlab == 6 && fmode(f_btn) == 3 && fmode(f_ind) == 3 && fmode(f_meter) == 0 && fmode(f_view) == 0);
    ok &= check("panel: dentro de lo oculto no se sigue mirando", fmode(f_icon) == 0);
    v_open(0x5555, "view/CAUTION", 0);   /* un aviso encima: lo oculto se mantiene */
    ((unsigned char *)f_btn)[0x19] = 0;
    ws_move(0x9001, 0, 2000, 2000, 0);
    ok &= check("panel: con un aviso encima se mantiene oculto lo que ya lo estaba", fmode(f_btn) == 3 && cxt_lab[0][6] == 0);
    v_close(0x5555, "view/CAUTION");
    ((unsigned char *)f_ind)[0x19] = 0;
    ws_move(0x9001, 0, 2000, 2000, 0);
    ok &= check("panel: al cerrarse el aviso se vuelve a recorrer", fmode(f_ind) == 3);
    say(ok ? "cxtest: panel TODO OK\n" : "cxtest: panel FALLA\n");
}

/* cxmode.txt = "panel 2\nhidebox 560 120 80 360\nhidew 1d0001\npaneldump 1" o vacío.
 * Zonas por omisión: «-» (572,262) 68x90, iris (110,418) 100x52, obturación (210,418) 120x52,
 * ganancia (330,418) 100x52, «+» (572,142) 68x90. */
static void panel_tests(int expect)
{
    int ok = 1;
    u32 a0, s0, p0, r0, m0;
    const u32 PX = 600, PY = 150, MX = 600, MY = 330, AX = 600, AY = 240, IX = 150, SX = 270, GX = 380, VY = 440;
    panel_tree();
    v_open(0x5555, "view/MOV_REC", 0);
    if (!expect) {
        tap(PX, PY);
        ok &= check("panel: sin la opción no se crea nada y los toques pasan",
                    cxt_nlab == 0 && cxt_mouse[1][5] == 1 && cxt_mouse[2][5] == 1 && cxt_mouse[0][5] == 2);
        ok &= check("panel: updateResource pasa intacta", ge_updres(0x100) == 0x107 && cxt_updres == 1);
        say(ok ? "cxtest: panel TODO OK\n" : "cxtest: panel FALLA\n");
        return;
    }
    ok &= check("panel: seis etiquetas en la capa física, pasivas, cada una en su zona",
                cxt_nlab == 6 && cxt_lab[0][1] == 0xa11a && cxt_lab[5][1] == 0xa11a &&
                cxt_lab[0][4] == (572u << 16) && cxt_lab[0][5] == (290u << 16) && cxt_lab[0][2] == (68u << 16) && cxt_lab[0][3] == (90u << 16) &&
                cxt_lab[1][4] == (110u << 16) && cxt_lab[1][5] == (418u << 16) && cxt_lab[2][2] == (120u << 16) &&
                cxt_lab[4][5] == (110u << 16) && cxt_lab[5][5] == (205u << 16) && cxt_lab[2][7] == 1);
    ok &= check("panel: la fuente medida en la cámara",
                cxt_labfont[1][0] == 0x2038000 && cxt_labfont[1][1] == 0x203e010 && cxt_labfont[1][2] == 0x203e003 &&
                cxt_labfont[1][3] == 0x2036029 && cxt_labfont[1][4] == 0x2038002);
    ok &= check("panel: textos iniciales, con el iris elegido",
                lab_is(0, "-") && lab_is(1, "[F--A]") && lab_is(2, "--A") && lab_is(3, "AGC") && lab_is(4, "+") &&
                lab_is(5, "AUTO") && cxt_drawn >= 6);
    if (!cxt_rootpath) {
        ok &= check("panel: sin llegar a la raíz de la capa todavía no hay qué recorrer", fmode(f_btn) == 0 && fmode(f_ind) == 0);
        gi_setimage((u32)f_icon, 0x02200001, 0);        /* la interfaz toca un icono nativo: se sube hasta su vista */
        ts_setstr((u32)f_ind + 0x144, 0x02042999);      /* y una etiqueta nativa de la misma vista */
        ws_move(0x9001, 0, 2000, 2000, 0);              /* cualquier llamada de la interfaz: se recorre */
    }
    ok &= check("panel: oculta lo nativo que cabe en la caja y lo pedido por ID; lo demás no",
                fmode(f_btn) == 3 && fmode(f_ind) == 3 && fmode(f_meter) == 0 && fmode(f_view) == 0);
    setmode(0xf7);                       /* la interfaz ya habló con la cámara */
    fno(280);
    shut(1, 60);
    notify(0x8588, 0x9706, 0x1ec, 0, 0);
    ok &= check("panel: muestra lo que avisa la cámara", lab_is(1, "[F2.8A]") && lab_is(2, "1/60A") && lab_is(3, "AGC12"));

    a0 = cxt_obj[0][3]; p0 = cxt_mouse[1][5]; r0 = cxt_mouse[2][5]; m0 = cxt_mouse[0][5];
    tap(PX, PY);                         /* + con el iris en automático */
    ok &= check("panel: el toque en el panel no llega a la interfaz (sólo los Move de antes y de después)",
                cxt_mouse[1][5] == p0 && cxt_mouse[2][5] == r0 && cxt_mouse[0][5] == m0 + 2);
    ok &= check("panel: el primer + sobre el iris automático sólo pide prioridad de iris",
                find_seen(a0, cxt_obj[0][3], 0x7558, 0x95c8, 0xf3) >= 0 && lab_is(1, "[F2.8]"));
    s0 = cxt_obj[1][3];
    tap(PX, PY);
    ok &= check("panel: el siguiente + da un paso de iris",
                cxt_obj[1][3] == s0 + 1 && cxt_sync_seen[0] == 0x6502 && cxt_sync_seen[1] == 0x9573 && (cxt_sync_seen[2] & 0xff) == 1);
    fno(310);
    ok &= check("panel: aparece el valor nuevo", lab_is(1, "[F3.1]"));
    tap(MX, MY);
    ok &= check("panel: - da el paso contrario", cxt_obj[1][3] == s0 + 2 && (cxt_sync_seen[2] & 0xff) == 0xff);

    tap(SX, VY);                         /* elige la obturación */
    ok &= check("panel: tocar otro valor lo elige", lab_is(2, "[1/60A]") && lab_is(1, "F3.1"));
    a0 = cxt_obj[0][3];
    tap(PX, PY);
    ok &= check("panel: con el iris manual, la obturación se fija sin cambiar la prioridad",
                find_seen(a0, cxt_obj[0][3], 0x75b6, 0x96d6, 0x1cd) >= 0 && find_seen(a0, cxt_obj[0][3], 0x7558, 0x95c8, 0xf4) < 0 &&
                lab_is(2, "[1/60]"));
    s0 = cxt_obj[1][3];
    tap(PX, PY);
    ok &= check("panel: paso de obturación en prioridad de iris",
                cxt_obj[1][3] == s0 + 1 && cxt_sync_seen[0] == 0x6504 && cxt_sync_seen[1] == 0x9574 && (cxt_sync_seen[2] & 0xff) == 1);
    shut(1, 90);
    ok &= check("panel: aparece la obturación nueva", lab_is(2, "[1/90]"));
    a0 = cxt_obj[0][3];
    tap(AX, AY);                         /* AUTO sobre la obturación elegida */
    ok &= check("panel: AUTO devuelve a automático el valor elegido",
                find_seen(a0, cxt_obj[0][3], 0x75b6, 0x96d6, 0x1cc) >= 0 && lab_is(2, "[1/90A]"));
    a0 = cxt_obj[0][3];
    tap(AX, AY);
    ok &= check("panel: AUTO otra vez lo vuelve a fijar", find_seen(a0, cxt_obj[0][3], 0x75b6, 0x96d6, 0x1cd) >= 0 && lab_is(2, "[1/90]"));
    a0 = cxt_obj[0][3];
    tap(SX, VY);                         /* tocar el valor elegido hace lo mismo */
    ok &= check("panel: tocar el valor elegido también lo devuelve a automático",
                find_seen(a0, cxt_obj[0][3], 0x75b6, 0x96d6, 0x1cc) >= 0 && lab_is(2, "[1/90A]"));
    tap(IX, VY);                         /* elige el iris */
    a0 = cxt_obj[0][3];
    tap(IX, VY);
    ok &= check("panel: con los dos en automático vuelve el modo automático",
                find_seen(a0, cxt_obj[0][3], 0x7558, 0x95c8, 0xf7) >= 0 && lab_is(1, "[F3.1A]"));

    tap(GX, VY);                         /* elige la ganancia */
    a0 = cxt_obj[0][3];
    tap(PX, PY);
    ok &= check("panel: + sobre la ganancia automática la fija donde está",
                find_seen(a0, cxt_obj[0][3], 0x759e, 0x9654, 0x199) >= 0 && find_seen(a0, cxt_obj[0][3], 0x759e, 0x9652, 4) >= 0 &&
                find_seen(a0, cxt_obj[0][3], 0x75ba, 0x96dc, 12) >= 0 && find_seen(a0, cxt_obj[0][3], 0x75ba, 0x96dd, 0x1d0) >= 0 &&
                lab_is(3, "[12dB]"));
    a0 = cxt_obj[0][3];
    tap(PX, PY);
    ok &= check("panel: + sube 3 dB", find_seen(a0, cxt_obj[0][3], 0x75ba, 0x96dc, 15) >= 0 && lab_is(3, "[15dB]"));
    a0 = cxt_obj[0][3];
    tap(GX, VY);
    ok &= check("panel: tocar la ganancia elegida la devuelve a automática",
                find_seen(a0, cxt_obj[0][3], 0x75ba, 0x96dd, 0x1d1) >= 0 && find_seen(a0, cxt_obj[0][3], 0x759e, 0x9654, 0x19a) >= 0 &&
                lab_is(3, "[AGC12]"));

    ((unsigned char *)f_btn)[0x19] = 0;  /* la vista vuelve a mostrar su botón */
    p0 = cxt_mouse[1][5];
    tap(300, 200);                       /* fuera del panel */
    ok &= check("panel: un toque fuera del panel pasa", cxt_mouse[1][5] == p0 + 1);
    ok &= check("panel: lo que la vista vuelve a mostrar se oculta otra vez", fmode(f_btn) == 3);
    v_open(0x5555, "view/GLOBAL_MENU", 1);
    ok &= check("panel: con el menú abierto se oculta", cxt_lab[0][6] == 3 && cxt_lab[4][6] == 3);
    p0 = cxt_mouse[1][5];
    tap(PX, PY);
    ok &= check("panel: y los toques pasan al menú", cxt_mouse[1][5] == p0 + 1);
    v_open(0x5555, "view/CAMERA_SETTING_MENU", 1);
    v_close(0x5555, "view/CAMERA_SETTING_MENU");
    ok &= check("panel: al cerrar el submenú sigue oculto, queda el menú principal", cxt_lab[0][6] == 3);
    v_open(0x5555, "view/CAMERA_SETTING_MENU", 1);
    v_open(0x5555, "view/IRIS_L", 0);    /* una pantalla cierra los menús sin avisar */
    cxt_sibling = 1;
    v_close(0x5555, "view/IRIS_L");
    ok &= check("panel: vuelve al cerrar la pantalla, y al frente", cxt_lab[0][6] == 0 && cxt_lab[0][8] >= 1 && cxt_lab[4][8] >= 1);
    v_open(0x5555, "view/GLOBAL_MENU", 1);
    v_open(0x5555, "view/SETUP_MENU", 1);
    v_open(0x5555, "view/PB_DEMO_CAM_SET", 0);   /* una lista de opciones: se abre dentro del menú */
    v_close(0x5555, "view/PB_DEMO_CAM_SET");
    ok &= check("panel: al cerrar una lista de opciones se vuelve al menú, sigue oculto", cxt_lab[0][6] == 3);
    v_close(0x5555, "view/SETUP_MENU");
    ok &= check("panel: queda el menú principal, sigue oculto", cxt_lab[0][6] == 3);
    v_close(0x5555, "view/GLOBAL_MENU");
    ok &= check("panel: vuelve al cerrar el menú principal", cxt_lab[0][6] == 0);
    v_open(0x5555, "view/CAUTION", 0);
    ok &= check("panel: un aviso pasajero no lo oculta", cxt_lab[0][6] == 0);
    v_open(0x5555, "view/STL_REC_CLASSIC_L", 0);
    ok &= check("panel: en modo foto no se muestra", cxt_lab[0][6] == 3);
    v_open(0x5555, "view/MOV_REC_L", 0);
    ok &= check("panel: vuelve con la pantalla de video", cxt_lab[0][6] == 0);
    ok &= check("panel: updateResource pasa intacta", ge_updres(0x100) == 0x107 && cxt_updres == 1);
    say(ok ? "cxtest: panel TODO OK\n" : "cxtest: panel FALLA\n");
}

/* cxmode.txt = "show c347c 12\nshow c347c 13\nhide c347c 17\nshow c347c 99\nhide c347c 300\nshow 12345 12" o vacío. */
static void menu_tests(int expect)
{
    int ok = 1;
    u32 r, i;
    r = md_init(0x5001, (u32)&m_root, m_inc, 0x5002);
    ok &= check("menu: initMenuData llega intacta y devuelve lo de la original",
                cxt_menu[0] == 0x5001 && cxt_menu[1] == (u32)&m_root && cxt_menu[2] == (u32)m_inc &&
                cxt_menu[3] == 0x5002 && cxt_menu[4] == 1 && r == 0x77);
    if (expect) {
        ok &= check("menu: show deja IRIS (12) y Shutter Speed (13) visibles",
                    m_inc[12 * 3 + 1] == 2 && m_inc[12 * 3 + 2] == 1 && m_inc[13 * 3 + 1] == 2 && m_inc[13 * 3 + 2] == 1);
        ok &= check("menu: hide pone Low Lux (17) en estado 0 sin tocar el byte válido", m_inc[17 * 3 + 1] == 0 && m_inc[17 * 3 + 2] == 1);
    } else {
        ok &= check("menu: sin reglas, los ocultos siguen ocultos",
                    m_inc[12 * 3 + 1] == 0 && m_inc[13 * 3 + 1] == 1 && m_inc[17 * 3 + 2] == 1);
    }
    ok &= check("menu: un ítem visible y elegido no cambia", m_inc[10 * 3 + 1] == 3 && m_inc[10 * 3 + 2] == 1);
    ok &= check("menu: un id que no está en el árbol no se escribe", m_inc[99 * 3 + 1] == 0 && m_inc[99 * 3 + 2] == 0);
    for (i = 0; i < sizeof(m_inc); i++) m_inc[i] = 0;
    md_init(0x5001, (u32)&m_other, m_inc, 0x5002);       /* una tabla que no está en el lugar conocido */
    ok &= check("menu: otra tabla, aunque tenga la misma forma, no se toca",
                m_inc[12 * 3 + 1] == 0 && m_inc[13 * 3 + 1] == 1 && m_inc[17 * 3 + 2] == 1);
    say(ok ? "cxtest: menu TODO OK\n" : "cxtest: menu FALLA\n");
}

static void view_tests(void)
{
    int ok = 1, i;
    u32 r = 0;
    nodes[0] = (u32)node; vec[0] = (u32)nodes; vec[1] = (u32)(nodes + 1); vec[2] = vec[1];
    plist[0] = (u32)vec; plist[1] = (u32)&refc;
    onodes[0] = (u32)onode; ovec[0] = (u32)onodes; ovec[1] = (u32)(onodes + 1); ovec[2] = ovec[1];
    oplist[0] = (u32)ovec; oplist[1] = (u32)&refc;

    r = v_open(0x1111, "view/IRIS_L", 7);
    ok &= check("openView: retorno", r == 0x0e11);
    ok &= check("openView: argumentos", cxt_args[0][0] == 0x1111 && cxt_args[0][2] == 7 && cxt_args[0][5] == 1);
    r = v_close(0x2222, "view/IRIS_L");
    ok &= check("closeView", r == 0xc105e && cxt_args[1][0] == 0x2222 && cxt_args[1][5] == 1);
    r = v_exec(0x3333, "model/CAMERA", 0x2d, (u32)plist);
    ok &= check("requestModelExecute", r == 0xe7ec && cxt_args[2][0] == 0x3333 && cxt_args[2][2] == 0x2d &&
                cxt_args[2][3] == (u32)plist && cxt_args[2][5] == 1);
    for (i = 0; i < 5; i++) {
        onode[3] = 0;
        r = v_info(0x4444, "model/CAMERA", 0xf, (u32)oplist, i == 4 ? (u32)plist : 0);
    }
    ok &= check("getModelInfo: 5 argumentos, el quinto por la pila",
                r == 1 && cxt_args[3][0] == 0x4444 && cxt_args[3][2] == 0xf && cxt_args[3][3] == (u32)oplist &&
                cxt_args[3][4] == (u32)plist && cxt_args[3][5] == 5 && onode[3] == 0x1234);
    say(ok ? "cxtest: view TODO OK\n" : "cxtest: view FALLA\n");
}

/* Con cxmode.txt = iris, abrir Exposure debe llegar como view/IRIS; lo demás, intacto. */
static void redirect_tests(int expect)
{
    int ok = 1;
    const char *got;
    const char *e;
    v_open(0x5555, "view/EXPOSURE", 0);
    got = (const char *)cxt_args[0][1];
    e = expect ? "view/IRIS" : "view/EXPOSURE";
    while (*got && *got == *e) { got++; e++; }
    ok &= check(expect ? "Exposure redirigida a view/IRIS" : "Exposure sin redirección", *got == *e);
    v_open(0x5555, "view/FOCUS", 0);
    got = (const char *)cxt_args[0][1];
    e = "view/FOCUS";
    while (*got && *got == *e) { got++; e++; }
    ok &= check("otras pantallas intactas", *got == *e);
    say(ok ? "cxtest: redirect TODO OK\n" : "cxtest: redirect FALLA\n");
}

static int streq_(const char *a, const char *b)
{
    while (*a && *a == *b) { a++; b++; }
    return *a == *b;
}

/*
 * v5, con cxmode.txt:
 *   redirect view/EXPOSURE view/IRIS
 *   redirect view/SPOT_METER view/SHUTTER_SPEED
 *   redirect view/SPOT_FOCUS view/NO_EXISTE     (ignorada)
 *   aemode 1
 */
static void v5_tests(void)
{
    int ok = 1;
    v_open(0x6666, "view/EXPOSURE", 0);
    ok &= check("Exposure -> Iris", streq_((const char *)cxt_args[0][1], "view/IRIS"));
    v_close(0x6666, "view/IRIS_L");
    v_open(0x6666, "view/SPOT_METER", 0);
    ok &= check("Spot Meter -> Shutter Speed", streq_((const char *)cxt_args[0][1], "view/SHUTTER_SPEED"));
    v_open(0x6666, "view/SPOT_FOCUS", 0);
    ok &= check("destino fuera de la lista: sin redirigir", streq_((const char *)cxt_args[0][1], "view/SPOT_FOCUS"));
    v_close(0x6666, "view/SPOT_FOCUS");
    v_open(0x6666, "view/CAUTION", 0);    /* un aviso encima no debe borrar el contexto */
    v_close(0x6666, "view/CAUTION");

    /* Modo AE: la pantalla de obturación pide 3, la cámara recibe 1. */
    node[2] = 0x12a; node[3] = 3;
    v_exec(0x6666, "model/CAMERA", 0x10, (u32)plist);
    ok &= check("aemode: 3 se manda como 1", cxt_exec_value == 1);
    node[3] = 5;
    v_exec(0x6666, "model/CAMERA", 0x10, (u32)plist);
    ok &= check("aemode: automático (5) pasa intacto", cxt_exec_value == 5);
    node[3] = 2;
    v_exec(0x6666, "model/DISPLAY", 0x10, (u32)plist);
    ok &= check("aemode: otro modelo pasa intacto", cxt_exec_value == 2);

    /* La cámara está en 1; la pantalla abierta (Shutter Speed) debe ver 3. */
    onode[2] = 0x12b;
    cxt_info_value = 1;
    v_info(0x6666, "model/CAMERA", 0xf, (u32)oplist, 0);
    ok &= check("con Shutter Speed abierta se reporta 3", onode[3] == 3);
    v_close(0x6666, "view/SHUTTER_SPEED_L");   /* el motor cierra con el nombre _L */
    v_info(0x6666, "model/CAMERA", 0xf, (u32)oplist, 0);
    ok &= check("sin pantalla manual abierta se reporta el real", onode[3] == 1);
    v_open(0x6666, "view/EXPOSURE", 0);
    v_info(0x6666, "model/CAMERA", 0xf, (u32)oplist, 0);
    ok &= check("con Iris abierta se reporta 2", onode[3] == 2);
    cxt_info_value = 6;
    v_info(0x6666, "model/CAMERA", 0xf, (u32)oplist, 0);
    ok &= check("modo de fábrica (6) pasa intacto", onode[3] == 6);
    say(ok ? "cxtest: v5 TODO OK\n" : "cxtest: v5 FALLA\n");
}

/* cxmode.txt = "aeauto 6": automático (5) se manda como 6; 2 pasa intacto. */
static void aeauto_tests(void)
{
    int ok = 1;
    nodes[0] = (u32)node; vec[0] = (u32)nodes; vec[1] = (u32)(nodes + 1); vec[2] = vec[1];
    plist[0] = (u32)vec; plist[1] = (u32)&refc;
    node[1] = 1; node[2] = 0x12a; node[3] = 5;
    v_exec(0x7777, "model/CAMERA", 0x10, (u32)plist);
    ok &= check("aeauto: 5 se manda como 6", cxt_exec_value == 6);
    node[3] = 2;
    v_exec(0x7777, "model/CAMERA", 0x10, (u32)plist);
    ok &= check("aeauto: prioridad de iris (2) pasa intacta", cxt_exec_value == 2);
    say(ok ? "cxtest: aeauto TODO OK\n" : "cxtest: aeauto FALLA\n");
}

void _start(void)
{
    long fd = cx_open("/proc/self/cmdline", O_RDONLY, 0);
    char buf[1024];
    long n = fd >= 0 ? cx_read(fd, buf, sizeof(buf)) : 0;
    if (fd >= 0) cx_close(fd);
    if (has_arg(buf, n, "init")) {
        mwf_inh_initialize();
        mwf_inh_or(0x174, 3, 0x80000001);   /* factores 0x60 y 0x7f */
        mwf_inh_or(0x402, 57, 1);           /* factor 0x720, última palabra */
    }
    if (has_arg(buf, n, "view")) view_tests();
    if (has_arg(buf, n, "redirect")) redirect_tests(1);
    if (has_arg(buf, n, "noredirect")) redirect_tests(0);
    if (has_arg(buf, n, "aeauto")) aeauto_tests();
    if (has_arg(buf, n, "obj")) obj_tests();
    if (has_arg(buf, n, "objmanual")) objmanual_tests(1);
    if (has_arg(buf, n, "hold")) hold_tests();
    if (has_arg(buf, n, "gain")) gain_tests();
    if (has_arg(buf, n, "fix")) fix_tests();
    if (has_arg(buf, n, "iso")) iso_tests();
    if (has_arg(buf, n, "expo")) expo_tests();
    if (has_arg(buf, n, "unlock")) unlock_tests(1);
    if (has_arg(buf, n, "nounlock")) unlock_tests(0);
    if (has_arg(buf, n, "auto")) auto_tests();
    if (has_arg(buf, n, "lock")) lock_tests();
    if (has_arg(buf, n, "text")) text_tests(1);
    if (has_arg(buf, n, "notext")) text_tests(0);
    if (has_arg(buf, n, "ux")) ux_tests(1);
    if (has_arg(buf, n, "noux")) ux_tests(0);
    if (has_arg(buf, n, "menu")) menu_tests(1);
    if (has_arg(buf, n, "nomenu")) menu_tests(0);
    if (has_arg(buf, n, "panelroot")) { cxt_rootpath = 1; panel_tests(1); }
    if (has_arg(buf, n, "panelkeep")) panelkeep_tests();
    if (has_arg(buf, n, "panelwb")) panelwb_tests();
    if (has_arg(buf, n, "panel")) panel_tests(1);
    if (has_arg(buf, n, "nopanel")) panel_tests(0);
    if (has_arg(buf, n, "noobjmanual")) objmanual_tests(0);
    if (has_arg(buf, n, "v5")) {
        nodes[0] = (u32)node; vec[0] = (u32)nodes; vec[1] = (u32)(nodes + 1); vec[2] = vec[1];
        plist[0] = (u32)vec; plist[1] = (u32)&refc;
        onodes[0] = (u32)onode; ovec[0] = (u32)onodes; ovec[1] = (u32)(onodes + 1); ovec[2] = ovec[1];
        oplist[0] = (u32)ovec; oplist[1] = (u32)&refc;
        v5_tests();
    }
    say("cxtest: esperando\n");
    cx_sleep(4);
    say("cxtest: fin\n");
    sys3(248 /* exit_group: termina también los hilos de cxg.so */, 0, 0, 0);
}
