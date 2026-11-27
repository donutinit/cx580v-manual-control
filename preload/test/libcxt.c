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
