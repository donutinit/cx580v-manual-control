/*
 * Hace de libObj.so en QEMU: define las funciones de ViewBase que cxview
 * envuelve, guarda los argumentos que recibe y devuelve valores conocidos.
 * getModelInfo escribe un valor en la lista de salida.
 */
typedef unsigned int u32;

u32 cxt_args[5][6];   /* [función][argumento], [i][5] = número de llamadas */
u32 cxt_info_value = 0x1234;   /* lo que getModelInfo escribe en la salida */
u32 cxt_exec_value;            /* valor del primer nodo que recibió requestModelExecute */

u32 t_open(u32 self, u32 name, u32 n) __asm__("_ZN8ViewBase8openViewEPKci");
u32 t_open(u32 self, u32 name, u32 n)
{ cxt_args[0][0] = self; cxt_args[0][1] = name; cxt_args[0][2] = n; cxt_args[0][5]++; return 0x0e11; }

u32 t_close(u32 self, u32 name) __asm__("_ZN8ViewBase9closeViewEPKc");
u32 t_close(u32 self, u32 name)
{ cxt_args[1][0] = self; cxt_args[1][1] = name; cxt_args[1][5]++; return 0xc105e; }

u32 t_exec(u32 self, u32 model, u32 action, u32 plist) __asm__("_ZN8ViewBase19requestModelExecuteEPKcmP9ParamList");
u32 t_exec(u32 self, u32 model, u32 action, u32 plist)
{
    cxt_args[2][0] = self; cxt_args[2][1] = model; cxt_args[2][2] = action; cxt_args[2][3] = plist; cxt_args[2][5]++;
    if (plist) {
        u32 *vec = (u32 *)((u32 *)plist)[0];
        cxt_exec_value = ((u32 *)((u32 *)vec[0])[0])[3];
    }
    return 0xe7ec;
}

u32 t_info(u32 self, u32 model, u32 id, u32 out, u32 in) __asm__("_ZN8ViewBase12getModelInfoEPKciP9ParamListPKS2_");
u32 t_info(u32 self, u32 model, u32 id, u32 out, u32 in)
{
    cxt_args[3][0] = self; cxt_args[3][1] = model; cxt_args[3][2] = id; cxt_args[3][3] = out; cxt_args[3][4] = in;
    cxt_args[3][5]++;
    /* out: ParamList -> vector -> primer nodo; su valor pasa a 0x1234 */
    if (out) {
        u32 *vec = (u32 *)((u32 *)out)[0];
        u32 *node = (u32 *)((u32 *)vec[0])[0];
        node[3] = cxt_info_value;
    }
    return 1;
}

/* MWF::ObjIf: guarda los argumentos; Sync escribe 0x1cb en el primer parámetro de out. */
u32 cxt_obj[2][4];
u32 t_async(u32 obj, u32 msg) __asm__("_ZN3MWF5ObjIf17IssueCommandAsyncEPvPNS_6ObjMsgE");
/* Copia de cada mensaje asíncrono: {cat, msg, cuenta, id0, val0, id1, val1, id2, val2}. */
u32 cxt_seen[16][9];
u32 cxt_async_ret = 0xa5;
/* Como InhFeatureCheck: si != 0, sync y async consultan GetInhInfo de esa función y devuelven 0xa000 si está bloqueada. */
u32 cxt_check_feature;
static int inh_blocked(u32 feature);
/* Último mensaje síncrono: {mid, id0, val0, cuenta}. */
u32 cxt_sync_seen[4];
u32 t_async(u32 obj, u32 msg)
{
    u32 n = cxt_obj[0][3] & 15, *body = (u32 *)*(u32 *)msg, *chunk, i;
    cxt_obj[0][0] = obj; cxt_obj[0][1] = msg; cxt_obj[0][3]++;
    for (i = 0; i < 9; i++) cxt_seen[n][i] = 0;
    if (body) {
        cxt_seen[n][0] = body[1]; cxt_seen[n][1] = body[2]; cxt_seen[n][2] = body[3];
        chunk = (u32 *)body[4];
        for (i = 0; i < 3 && i < body[3] && chunk; i++) {
            cxt_seen[n][3 + 2 * i] = chunk[1 + 3 * i];
            cxt_seen[n][4 + 2 * i] = chunk[2 + 3 * i];
        }
    }
    return cxt_async_ret;
}
u32 t_sync(u32 obj, u32 in, u32 out) __asm__("_ZN3MWF5ObjIf16IssueCommandSyncEPvPNS_6ObjMsgES3_");
u32 t_sync(u32 obj, u32 in, u32 out)
{
    if (cxt_check_feature && inh_blocked(cxt_check_feature)) return 0xa000;
    cxt_obj[1][0] = obj; cxt_obj[1][1] = in; cxt_obj[1][2] = out; cxt_obj[1][3]++;
    if (in) {
        u32 *body = (u32 *)*(u32 *)in;
        if (body) {
            u32 *chunk = (u32 *)body[4];
            cxt_sync_seen[0] = body[2];
            cxt_sync_seen[3] = body[3];
            cxt_sync_seen[1] = chunk ? chunk[1] : 0;
            cxt_sync_seen[2] = chunk ? chunk[2] : 0;
        }
    }
    if (out && *(u32 *)out && ((u32 *)*(u32 *)out)[4]) {
        u32 *body = (u32 *)*(u32 *)out;
        u32 *chunk = (u32 *)body[4];
        chunk[2] = 0x1cb;
    }
    return 0x5c;
}

/*
 * InhInfo::GetInhInfo falso (la real necesita /dev/uipc): llena el mensaje como
 * SubMakeInhFactorMsg, con el bool 0x1000 = bloqueada si la función está en la
 * lista de cxt_inh_blocked. Usa las funciones de la libMWF.so de la cámara.
 */
void mwf_add(u32 *m, u32 id, u32 *p) __asm__("_ZN3MWF6ObjMsg8AddParamEjRKNS_5ParamE");
void mwf_bool_ctor(u32 *p, int v) __asm__("_ZN3MWF17ParamStd_TemplateIbLNS_5Param4TypeE1EEC1Eb");
void mwf_bool_dtor(u32 *p) __asm__("_ZN3MWF17ParamStd_TemplateIbLNS_5Param4TypeE1EED1Ev");
u32 cxt_inh_blocked[4] = { 0x116, 0x174, 0, 0 };
void t_getinh(u32 feature, u32 *msg) __asm__("_ZN3MWF7InhInfo10GetInhInfoEN6DefInh11FeatureId_tEPNS_6ObjMsgE");
void t_getinh(u32 feature, u32 *msg)
{
    u32 p[4] __attribute__((aligned(8)));
    int i, b = 0;
    for (i = 0; i < 4; i++) { p[i] = 0; if (cxt_inh_blocked[i] && cxt_inh_blocked[i] == feature) b = 1; }
    mwf_bool_ctor(p, b);
    mwf_add(msg, 0x1000, p);
    mwf_bool_dtor(p);
}

void mwf_msg_ctor0_s(u32 *m) __asm__("_ZN3MWF6ObjMsgC1Ev");
void mwf_msg_dtor_s(u32 *m) __asm__("_ZN3MWF6ObjMsgD1Ev");
u32 mwf_get_s(u32 *m, u32 id) __asm__("_ZNK3MWF6ObjMsg8GetParamEj");
/* Pregunta por la PLT (pasa por la envoltura de cxg.so, como en libObj). */
static int inh_blocked(u32 feature)
{
    u32 m[4] __attribute__((aligned(8)));
    u32 q;
    int i, v = 0;
    for (i = 0; i < 4; i++) m[i] = 0;
    mwf_msg_ctor0_s(m);
    t_getinh(feature, m);
    q = mwf_get_s(m, 0x1000);
    if (q) v = (*(u32 *)q & 0xff) != 0;
    mwf_msg_dtor_s(m);
    return v;
}

/*
 * ux::gfxeng::TextSprite::setString(unsigned int) y ResourceManagerImpl::getText
 * falsos. En la cámara son Thumb, así que aquí también: prueba que cxg.so los
 * encuentra (st_value impar) y los llama cambiando de modo.
 */
u32 cxt_text[2][3];   /* [0] setString: {self, id, cuenta}; [1] getText: {self, id, cuenta} */
__attribute__((target("thumb"), noinline))
void t_setstr(u32 self, u32 id) __asm__("_ZN2ux6gfxeng10TextSprite9setStringEj");
void t_setstr(u32 self, u32 id) { cxt_text[0][0] = self; cxt_text[0][1] = id; cxt_text[0][2]++; }
__attribute__((target("thumb"), noinline))
u32 t_gettext(u32 self, u32 id, u32 out) __asm__("_ZN2ux6resmgr19ResourceManagerImpl7getTextEjRNS_4core7UStringE");
u32 t_gettext(u32 self, u32 id, u32 out) { cxt_text[1][0] = self; cxt_text[1][1] = id; cxt_text[1][2]++; return out + 1; }

/* GEN_Icon::setImage(unsigned int, bool) falso (en la cámara es ARM). */
u32 cxt_image[4];   /* {self, id, flag, cuenta} */
void t_setimage(u32 self, u32 id, u32 flag) __asm__("_ZN8GEN_Icon8setImageEjb");
void t_setimage(u32 self, u32 id, u32 flag) { cxt_image[0] = self; cxt_image[1] = id; cxt_image[2] = flag; cxt_image[3]++; }

/*
 * GEN_Label y Widget falsos para el panel propio (cxpanel.c). Cada etiqueta que
 * se construye queda en la lista: {this, capa, ancho, alto, x, y, modo, pasiva,
 * veces al frente, textos puestos}. ARM o Thumb como en la cámara.
 */
#define CXT_NLAB 8
u32 cxt_lab[CXT_NLAB][10];
u32 cxt_labfont[CXT_NLAB][5];
char cxt_labtext[CXT_NLAB][20];
u32 cxt_nlab, cxt_sibling, cxt_drawn;
static int lab_of(u32 w)
{
    u32 i;
    for (i = 0; i < cxt_nlab; i++) if (cxt_lab[i][0] == w) return (int)i;
    return -1;
}
void t_lab_ctor(u32 self) __asm__("_ZN9GEN_LabelC1Ev");
/* Con cxt_rootpath, la etiqueta queda colgada de la raíz como en el framework:
 * +8 apunta al nodo del padre, y ese nodo (+0) a su widget dueño. */
u32 cxt_rootpath, cxt_rootnode[2];
void t_lab_ctor(u32 self)
{
    if (cxt_nlab < CXT_NLAB) cxt_lab[cxt_nlab++][0] = self;
    if (cxt_rootpath) ((u32 *)self)[2] = (u32)cxt_rootnode;
}
void t_lab_size(u32 self, u32 w, u32 h) __asm__("_ZN9GEN_Label7setSizeEN2ux4core7Fixed32ES2_");
void t_lab_size(u32 self, u32 w, u32 h) { int i = lab_of(self); if (i >= 0) { cxt_lab[i][2] = w; cxt_lab[i][3] = h; } }
#define LABFONT(name, sym, k) void name(u32 self, u32 v) __asm__(sym); \
    void name(u32 self, u32 v) { int i = lab_of(self); if (i >= 0) cxt_labfont[i][k] = v; }
LABFONT(t_lab_font, "_ZN9GEN_Label7setFontEj", 0)
LABFONT(t_lab_fsize, "_ZN9GEN_Label11setFontSizeEj", 1)
LABFONT(t_lab_fstyle, "_ZN9GEN_Label12setFontStyleEj", 2)
LABFONT(t_lab_halign, "_ZN9GEN_Label18setHorizontalAlignEj", 3)
LABFONT(t_lab_dir, "_ZN9GEN_Label16setDirectionTypeEj", 4)
__attribute__((target("thumb"), noinline))
u32 t_genid(const char *s) __asm__("_ZN2ux4core2ID5genIDEPKc");
u32 t_genid(const char *s) { return s[0] == 'A' && s[1] == '_' && s[2] == 'P' ? 0xa11a : 0xbad; }
__attribute__((target("thumb"), noinline))
void t_w_layer(u32 self, const u32 *id) __asm__("_ZN2ux6wgtsys6Widget10setLayerIDERKNS_4core2IDE");
void t_w_layer(u32 self, const u32 *id) { int i = lab_of(self); if (i >= 0) cxt_lab[i][1] = *id; }
__attribute__((target("thumb"), noinline))
void t_w_pos(u32 self, const u32 *xy) __asm__("_ZN2ux6wgtsys6Widget11setLocalPosERKNS_4core7Vector2E");
void t_w_pos(u32 self, const u32 *xy) { int i = lab_of(self); if (i >= 0) { cxt_lab[i][4] = xy[0]; cxt_lab[i][5] = xy[1]; } }
__attribute__((target("thumb"), noinline))
void t_w_last(u32 self) __asm__("_ZN2ux6wgtsys6Widget8moveLastEv");
void t_w_last(u32 self) { int i = lab_of(self); if (i >= 0) cxt_lab[i][8]++; }
/* Árbol falso que arma la prueba: {raíz, vista, botón, icono (hijo del botón), indicador, medidor}. */
u32 cxt_tree[6];
__attribute__((target("thumb"), noinline))
u32 t_w_next(u32 self) __asm__("_ZNK2ux6wgtsys6Widget14getNextSiblingEv");
u32 t_w_next(u32 self)
{
    if (lab_of(self) >= 0) return cxt_sibling;
    if (self && self == cxt_tree[2]) return cxt_tree[4];
    if (self && self == cxt_tree[4]) return cxt_tree[5];
    return 0;
}
__attribute__((target("thumb"), noinline))
u32 t_w_parent(u32 self) __asm__("_ZNK2ux6wgtsys6Widget9getParentEv");
u32 t_w_parent(u32 self)
{
    /* Como en la cámara: un hijo directo de la raíz (las etiquetas, la vista) «no tiene padre». */
    if (!self || lab_of(self) >= 0) return 0;
    if (self == cxt_tree[3]) return cxt_tree[2];
    if (self == cxt_tree[2] || self == cxt_tree[4] || self == cxt_tree[5]) return cxt_tree[1];
    return 0;
}
__attribute__((target("thumb"), noinline))
u32 t_w_child(u32 self) __asm__("_ZNK2ux6wgtsys6Widget13getFirstChildEv");
u32 t_w_child(u32 self)
{
    if (!self) return 0;
    if (self == cxt_tree[0]) return cxt_tree[1];
    if (self == cxt_tree[1]) return cxt_tree[2];
    if (self == cxt_tree[2]) return cxt_tree[3];
    return 0;
}
__attribute__((target("thumb"), noinline))
void t_w_mode(u32 self, u32 mode) __asm__("_ZN2ux6wgtsys6Widget7setModeENS1_10WidgetModeE");
void t_w_mode(u32 self, u32 mode)
{
    int i = lab_of(self);
    if (i >= 0) cxt_lab[i][6] = mode;
    else ((unsigned char *)self)[0x19] = (unsigned char)mode;
}
__attribute__((target("thumb"), noinline))
void t_w_passive(u32 self, u32 on) __asm__("_ZN2ux6wgtsys6Widget16setPassiveRegionEb");
void t_w_passive(u32 self, u32 on) { int i = lab_of(self); if (i >= 0) cxt_lab[i][7] = on; }
__attribute__((target("thumb"), noinline))
u32 t_ts_draw(u32 sprite) __asm__("_ZN2ux6gfxeng10TextSprite8drawTextEv");
u32 t_ts_draw(u32 sprite) { (void)sprite; cxt_drawn++; return 0; }
/* GraphicsEngine::updateResource() falso. */
u32 cxt_updres;
__attribute__((target("thumb"), noinline))
u32 t_updres(u32 self) __asm__("_ZN2ux6gfxeng14GraphicsEngine14updateResourceEv");
u32 t_updres(u32 self) { cxt_updres++; return self + 7; }

/* TextSprite::setString(char const*) falso (Thumb, como en la cámara). */
u32 cxt_textc[3];   /* {self, primeros 4 bytes del texto, cuenta} */
__attribute__((target("thumb"), noinline))
void t_setstr_c(u32 self, const char *s) __asm__("_ZN2ux6gfxeng10TextSprite9setStringEPKc");
void t_setstr_c(u32 self, const char *s)
{
    int lab = lab_of(self - 0x144), k;
    if (lab >= 0) {                 /* etiqueta del panel: se guarda el texto entero */
        for (k = 0; k < 19 && s[k]; k++) cxt_labtext[lab][k] = s[k];
        cxt_labtext[lab][k] = 0;
        cxt_lab[lab][9]++;
        return;
    }
    cxt_textc[0] = self;
    cxt_textc[1] = (u32)(unsigned char)s[0] | (u32)(unsigned char)s[1] << 8 | (u32)(unsigned char)s[2] << 16 | (u32)(unsigned char)s[3] << 24;
    cxt_textc[2]++;
}

/* WidgetSystem::postMouse* y EventManager::push falsos: guardan todo lo que llega. */
u32 cxt_mouse[3][6];   /* [0] move {ws, mouse, x, y, 5º argumento (pila), cuenta}; [1] press; [2] release */
u32 t_mmove(u32 ws, u32 mouse, u32 x, u32 y, u32 t) __asm__("_ZN2ux6wgtsys12WidgetSystem13postMouseMoveEhssNS_4core8MsecTimeE");
u32 t_mmove(u32 ws, u32 mouse, u32 x, u32 y, u32 t)
{ cxt_mouse[0][0] = ws; cxt_mouse[0][1] = mouse; cxt_mouse[0][2] = x; cxt_mouse[0][3] = y; cxt_mouse[0][4] = t; cxt_mouse[0][5]++; return 0x111; }
__attribute__((target("thumb"), noinline))
u32 t_mpress(u32 ws, u32 mouse, u32 button, u32 t) __asm__("_ZN2ux6wgtsys12WidgetSystem14postMousePressEhhNS_4core8MsecTimeE");
u32 t_mpress(u32 ws, u32 mouse, u32 button, u32 t)
{ cxt_mouse[1][0] = ws; cxt_mouse[1][1] = mouse; cxt_mouse[1][2] = button; cxt_mouse[1][3] = t; cxt_mouse[1][5]++; return 0x222; }
u32 t_mrel(u32 ws, u32 mouse, u32 button, u32 t) __asm__("_ZN2ux6wgtsys12WidgetSystem16postMouseReleaseEhhNS_4core8MsecTimeE");
u32 t_mrel(u32 ws, u32 mouse, u32 button, u32 t)
{ cxt_mouse[2][0] = ws; cxt_mouse[2][1] = mouse; cxt_mouse[2][2] = button; cxt_mouse[2][3] = t; cxt_mouse[2][5]++; return 0x333; }
u32 cxt_push[3];       /* {self, ev, cuenta} */
u32 t_evpush(u32 self, u32 ev) __asm__("_ZN12EventManager4pushEP5Event");
u32 t_evpush(u32 self, u32 ev) { cxt_push[0] = self; cxt_push[1] = ev; cxt_push[2]++; return 0; }

/* CmnViewMenuData::initMenuData falso: hace de «exclusivo» y deja estados de fábrica. */
u32 cxt_menu[5];   /* {self, root, inc, excl, cuenta} */
u32 t_initmenu(u32 self, u32 root, unsigned char *inc, u32 excl)
    __asm__("_ZN15CmnViewMenuData12initMenuDataEP21CmnViewMenuItemStructP26CmnViewMenuItemInconstPartP22CmnViewMenuExclusiveIF");
u32 t_initmenu(u32 self, u32 root, unsigned char *inc, u32 excl)
{
    cxt_menu[0] = self; cxt_menu[1] = root; cxt_menu[2] = (u32)inc; cxt_menu[3] = excl; cxt_menu[4]++;
    inc[10 * 3 + 1] = 3; inc[10 * 3 + 2] = 1;     /* Exposure: normal y elegido */
    inc[12 * 3 + 1] = 0; inc[12 * 3 + 2] = 1;     /* IRIS: oculto por el exclusivo */
    inc[13 * 3 + 1] = 1; inc[13 * 3 + 2] = 1;     /* Shutter Speed: oculto */
    inc[17 * 3 + 1] = 2; inc[17 * 3 + 2] = 1;     /* Low Lux: normal */
    return 0x77;
}
