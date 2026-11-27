/*
 * cxobj: anota los mensajes que la app manda a los objetos de cámara, sin
 * cambiarlos.
 *
 * cxg.so exporta MWF::ObjIf::IssueCommandAsync e IssueCommandSync con el mismo
 * nombre que libMWF.so; libObj.so las llama por la PLT, así que pasan primero
 * por aquí. Sólo se anotan los de la categoría de cámara (0x3100).
 *
 *   OA <obj> c=<cat> m=<msg> n=<params> p<id>:<tipo>=<valor> ...   asíncrono
 *   OS ... -> ret=<r> out p<id>:<tipo>=<valor> ...                 síncrono
 *
 * Los mensajes repetidos con los mismos parámetros sólo se anotan cuando
 * cambian (hasta MAX_CHANGES veces por mensaje). Los nombres de mensajes,
 * parámetros y valores están en firmware/names/.
 *
 * ObjMsg (libMWF.so 1.07): ObjMsg[0] = cuerpo; cuerpo+4 categoría, +8 mensaje,
 * +0xc ParamMap {cuenta, primer bloque}. Bloque: [0] siguiente, luego 5
 * entradas {id, Param}. Param = {valor, puntero al tipo}; tipo[0] es el número
 * de tipo (7 entero, 8 sin signo, 10 puntero...). Para enteros el valor va
 * directo en Param[0].
 */

#include "cx.h"

#ifndef CX_OBJ_LIB
#define CX_OBJ_LIB "/libMWF.so"
#endif
#define SYM_ASYNC "_ZN3MWF5ObjIf17IssueCommandAsyncEPvPNS_6ObjMsgE"
#define SYM_SYNC  "_ZN3MWF5ObjIf16IssueCommandSyncEPvPNS_6ObjMsgES3_"
#define CAT_CAMERA 0x3100

/* Diccionarios de libObj.so 1.07 (firmware/names/). */
#define MSG_SET_EXPOSURE_MODE 0x7558
#define P_EXPOSURE_MODE       0x95c8
#define V_EXP_MODE_IRIS       0xf3
#define V_EXP_MODE_SHUTTER    0xf4
#define V_EXP_MODE_MANUAL     0xf5
#define MSG_SET_IRIS_DRIVE_MODE 0x75b4
#define MSG_SET_SHUT_DRIVE_MODE 0x75b6
#define P_IRIS_DRIVE_MODE     0x96d3
#define P_SHUT_DRIVE_MODE     0x96d6
#define V_IRIS_MANUAL         0x1cb
#define V_SHUT_MANUAL         0x1cd
#define V_IRIS_AUTO           0x1ca
#define V_SHUT_AUTO           0x1cc
#ifndef CX_PEND_MS
#define CX_PEND_MS 80
#endif
#ifndef CX_FIX_SETTLE_MS
#define CX_FIX_SETTLE_MS 250      /* tras el cambio de modo, antes del primer paso */
#endif
#ifndef CX_FIX_WAIT_MS
#define CX_FIX_WAIT_MS 400        /* espera de la respuesta de cada paso */
#endif
#ifndef CX_FIX_WINDOW_MS
#define CX_FIX_WINDOW_MS 4000
#endif
#ifndef CX_FIX_ARMED_MS
#define CX_FIX_ARMED_MS 1500      /* tras el cambio, tiempo que se vigila aunque el valor esté bien */
#endif
#define MSG_SYNC_IRIS_DRIVE   0x6502   /* paso de iris: SIGNED char (tipo 3) */
#define MSG_SYNC_SHUT_DRIVE   0x6504
#define P_IRIS_DRIVE          0x9573
#define P_SHUT_DRIVE          0x9574
#define P_FNO                 0x951b   /* unsigned short (tipo 6), F x10 */
#define P_FNO_CUR_MIN         0x9787   /* F mínimo según el zoom */
#define P_SHUT_NUMER          0x951c
#define P_SHUT_DENOM          0x951d
#define MSG_SET_MANUAL_EXPOSURE_MODE 0x7552   /* la Exposure de fábrica */
#define P_MANUAL_EXPOSURE_MODE 0x95bd
#define V_MANUAL_EXP_ON       0xea
#define V_MANUAL_EXP_OFF      0xeb
#define MSG_SYNC_MANUAL_EXPOSURE_DRIVE 0x6506
#define P_MANUAL_EXPOSURE_DRIVE 0x9575
#define P_EXPOSURE_POS        0x9555
#define MSG_SET_AELOCK_MODE   0x7610   /* el que arma ObjCamera para Spot Meter */
#define P_AELOCK_MODE         0x9795   /* int (tipo 7) */
#define V_AELOCK_ON           0x2d6    /* NORMAL_AELOCK_ON */
#define V_AELOCK_OFF          0x2d8
#define MSG_SET_EXPOSURE_BIAS 0x759e
#define P_EXPOSURE_BIAS       0x9652
#define P_EXPOSURE_BIAS_SW    0x9654
#define V_EXPOSURE_BIAS_ON    0x199
#define V_EXPOSURE_BIAS_OFF   0x19a
#define MSG_SET_AGC_LIMIT     0x75ba
#define P_AGC_LIMIT           0x96dc   /* signed char, dB, múltiplo de 3 */
#define P_AGC_LIMIT_MODE      0x96dd
#define V_AGC_LIMIT_ON        0x1d0
#define V_AGC_LIMIT_OFF       0x1d1
#define P_EXPOSURE_BIAS_SET   0x9655   /* lo que lee la pantalla (signed char) */
#define P_EXPOSURE_BIAS_ACT   0x9658
#define MSG_SET_ISO_SPEED_RATE 0x75d0  /* {ISO, MIN, MAX}, VALUEID, int (pvt_SetParamIsoSpeedRate) */
#define P_ISO_SPEED_RATE      0x951e
#define P_ISO_SPEED_RATE_MIN  0x96fe
#define P_ISO_SPEED_RATE_MAX  0x96ff
#define V_ISO_AUTO            0x21
#define V_ISO_100             0x2c     /* cada VALUEID siguiente es 1/3 de paso */
#define V_ISO_6400            0x3e
#define P_ISO_GAIN_STATUS     0x9706
#define P_ISO_GAIN_VALID      0x9720
#define P_ISO_SPEED_RATE_ACT  0x9703

/* Para armar mensajes propios, con las funciones de libMWF.so. */
#ifndef CX_MWF_LIB
#define CX_MWF_LIB "/libMWF.so"
#endif
static const char *const mwf_syms[] = {
    "_ZN3MWF6ObjMsgC1Ejj",                                  /* ObjMsg(cat, msg) */
    "_ZN3MWF6ObjMsgD1Ev",                                   /* ~ObjMsg() */
    "_ZN3MWF6ObjMsg8AddParamEjRKNS_5ParamE",                /* AddParam(id, const Param&) */
    "_ZN3MWF6ObjMsg19SetCommonRelayParamEjPKNS_5ParamE",    /* SetCommonRelayParam(n, const Param*) */
    "_ZN3MWF17ParamStd_TemplateIiLNS_5Param4TypeE7EEC1Ei",  /* ParamStd<int,7>(v) */
    "_ZN3MWF17ParamStd_TemplateIiLNS_5Param4TypeE7EED1Ev",
    "_ZN3MWF17ParamStd_TemplateIjLNS_5Param4TypeE8EEC1Ej",  /* ParamStd<uint,8>(v) */
    "_ZN3MWF17ParamStd_TemplateIjLNS_5Param4TypeE8EED1Ev",
    "_ZN3MWF17ParamStd_TemplateIaLNS_5Param4TypeE3EEC1Ea",  /* ParamStd<signed char,3>(v) */
    "_ZN3MWF17ParamStd_TemplateIaLNS_5Param4TypeE3EED1Ev",
    "_ZN3MWF6ObjMsgC1Ev",                                   /* ObjMsg() para la respuesta */
};
enum { F_MSG_CTOR, F_MSG_DTOR, F_ADD, F_RELAY, F_INT_CTOR, F_INT_DTOR, F_UINT_CTOR, F_UINT_DTOR,
       F_SCHAR_CTOR, F_SCHAR_DTOR, F_MSG_CTOR0, NF };
static u32 mwf[NF];

static int resolve_mwf(void)
{
    u32 base;
    int i;
    if (mwf[NF - 1]) return 1;
    base = cx_lib_base(CX_MWF_LIB);
    for (i = 0; i < NF; i++)
        if (!(mwf[i] = cx_dlsym(base, mwf_syms[i]))) return 0;
    return 1;
}

typedef u32 (*fn2_t)(u32, u32);
typedef u32 (*fn3_t)(u32, u32, u32);

static u32 r_async, r_sync;
volatile int cx_manual_active;
static u32 last_exp_mode, camera_obj;
static u32 camera_mode;   /* EXPOSURE_MODE que de verdad recibió la cámara */
static int expo_on, expo_k;   /* expogain: exposición manual encendida y último paso */
/* gainlock: el AE Shift que eligió la pantalla, mientras a la cámara le llega +N. */
static volatile int lock_on, lock_k;
/*
 * autounlock: mientras corre una orden propia (our_call), GetInhInfo anota las
 * funciones que contesta bloqueadas. Si la orden vuelve con 0xa000, ésas pasan a
 * contestarse libres y la orden se reintenta una vez.
 */
#define V_INHIBITED 0xa000
#define NUNLOCK 8
static volatile int our_call;
static volatile u32 rej_feat[NUNLOCK];
static volatile int n_rej;
static u32 unlocked[NUNLOCK];
static int n_unlocked;
static int learn_unlock(void)
{
    struct line l;
    int i, j, added = 0;
    for (i = 0; i < n_rej && i < NUNLOCK; i++) {
        for (j = 0; j < n_unlocked && unlocked[j] != rej_feat[i]; j++) ;
        if (j < n_unlocked || n_unlocked >= NUNLOCK) continue;
        unlocked[n_unlocked++] = rej_feat[i];
        added++;
        if (cx_log_enabled()) {
            cxl_begin(&l, "UL");
            cxl_lit(&l, " aprendida función ");
            cxl_hex(&l, rej_feat[i]);
            cxl_end(&l);
        }
    }
    return added;
}
volatile int cx_iris_held, cx_shutter_held;

static void resolve(void)
{
    u32 base;
    if (r_async && r_sync) return;
    base = cx_lib_base(CX_OBJ_LIB);
    r_async = cx_dlsym(base, SYM_ASYNC);
    r_sync = cx_dlsym(base, SYM_SYNC);
}

static int msg_ids(u32 msg, u32 *cat, u32 *mid)
{
    const u32 *body;
    if (!msg || (msg & 3)) return 0;
    body = (const u32 *)*(const u32 *)msg;
    if (!body || ((u32)body & 3)) return 0;
    *cat = body[1];
    *mid = body[2];
    return 1;
}

/* Recorre los parámetros. Con l != 0 los anota; devuelve una firma. */
static u32 walk(u32 msg, struct line *l)
{
    const u32 *body, *pm, *chunk;
    u32 count, i, k = 0, h = 2166136261u;
    if (!msg || (msg & 3)) return h;
    body = (const u32 *)*(const u32 *)msg;
    if (!body || ((u32)body & 3)) return h;
    pm = body + 3;
    count = pm[0];
    chunk = (const u32 *)pm[1];
    if (count > 32) count = 32;
    for (i = 0; i < count && chunk && !((u32)chunk & 3); i++) {
        const u32 *e = chunk + 1 + 3 * k;
        u32 id = e[0], val = e[1], type = 0;
        const u32 *t = (const u32 *)e[2];
        if (t && !((u32)t & 3)) type = t[0];
        h = (h ^ id) * 16777619u;
        h = (h ^ val) * 16777619u;
        if (l) {
            cxl_lit(l, " p");
            cxl_hex(l, id);
            cxl_lit(l, ":");
            cxl_hex(l, type);
            cxl_lit(l, "=");
            cxl_hex(l, val);
        }
        if (++k > 4) { k = 0; chunk = (const u32 *)chunk[0]; }
    }
    return h;
}

#define SEEN 256
#define MAX_CHANGES 60
static struct { u32 key, sig, logged; } seen[SEEN];

static int changed(u32 key, u32 sig)
{
    u32 h = (key * 2654435761u) & (SEEN - 1), i;
    for (i = 0; i < SEEN; i++, h = (h + 1) & (SEEN - 1)) {
        if (seen[h].logged && seen[h].key == key) {
            if (seen[h].sig == sig || seen[h].logged >= MAX_CHANGES) return 0;
            seen[h].sig = sig;
            seen[h].logged++;
            return 1;
        }
        if (!seen[h].logged) {
            seen[h].key = key;
            seen[h].sig = sig;
            seen[h].logged = 1;
            return 1;
        }
    }
    return 0;
}

static void head(struct line *l, const char *kind, u32 obj, u32 cat, u32 mid, u32 msg)
{
    const u32 *body = (const u32 *)*(const u32 *)msg;
    cxl_begin(l, kind);
    cxl_lit(l, " ");
    cxl_hex(l, obj);
    cxl_lit(l, " c=");
    cxl_hex(l, cat);
    cxl_lit(l, " m=");
    cxl_hex(l, mid);
    cxl_lit(l, " n=");
    cxl_hex(l, body[3]);
}

/* Primer parámetro entero (tipo 3, 7 u 8) con ese id, o 0. */
static u32 *find_param(u32 msg, u32 pid)
{
    const u32 *body, *pm;
    u32 *chunk;
    u32 count, i, k = 0;
    if (!msg || (msg & 3)) return 0;
    body = (const u32 *)*(const u32 *)msg;
    if (!body || ((u32)body & 3)) return 0;
    pm = body + 3;
    count = pm[0];
    chunk = (u32 *)pm[1];
    if (count > 32) count = 32;
    for (i = 0; i < count && chunk && !((u32)chunk & 3); i++) {
        u32 *e = chunk + 1 + 3 * k;
        const u32 *t = (const u32 *)e[2];
        if (e[0] == pid && t && !((u32)t & 3) && (t[0] == 3 || t[0] == 7 || t[0] == 8)) return &e[1];
        if (++k > 4) { k = 0; chunk = (u32 *)chunk[0]; }
    }
    return 0;
}

/*
 * Manda a la capa de cámara un mensaje propio, como lo hace ObjCamera
 * (pvt_ActionSetShutterDriveMode): relevo 1 con un número de secuencia y los
 * parámetros. Corre en el hilo de la app que llamó a la envoltura, así que las
 * funciones de libMWF se usan con su TLS normal.
 */
struct iparam { u32 id, type, value; };   /* type: 7 int, 8 uint, 3 signed char */

static u32 inject_n(u32 obj, u32 mid, const struct iparam *p, int n)
{
    static u32 seq = 0xcc000000u;
    u32 msg[4] __attribute__((aligned(8)));
    u32 prm[4] __attribute__((aligned(8)));
    struct line l;
    u32 r, i;
    int k;
    if (!obj || !r_async || !resolve_mwf()) return 0xffffffffu;
    for (i = 0; i < 4; i++) msg[i] = prm[i] = 0;
    ((void (*)(u32, u32, u32))mwf[F_MSG_CTOR])((u32)msg, CAT_CAMERA, mid);
    ((void (*)(u32, u32))mwf[F_UINT_CTOR])((u32)prm, ++seq);
    ((void (*)(u32, u32, u32))mwf[F_RELAY])((u32)msg, 1, (u32)prm);
    ((void (*)(u32))mwf[F_UINT_DTOR])((u32)prm);
    for (k = 0; k < n; k++) {
        int c = p[k].type == 8 ? F_UINT_CTOR : p[k].type == 3 ? F_SCHAR_CTOR : F_INT_CTOR;
        for (i = 0; i < 4; i++) prm[i] = 0;
        ((void (*)(u32, u32))mwf[c])((u32)prm, p[k].value);
        ((void (*)(u32, u32, u32))mwf[F_ADD])((u32)msg, p[k].id, (u32)prm);
        ((void (*)(u32))mwf[c + 1])((u32)prm);
    }
    if (cx_log_enabled()) {
        head(&l, "IN", obj, CAT_CAMERA, mid, (u32)msg);
        walk((u32)msg, &l);
    }
    n_rej = 0;
    our_call = 1;
    r = ((fn2_t)r_async)(obj, (u32)msg);
    our_call = 0;
    if (r == V_INHIBITED && cx_autounlock && learn_unlock()) {
        n_rej = 0;
        our_call = 1;
        r = ((fn2_t)r_async)(obj, (u32)msg);
        our_call = 0;
    }
    if (cx_log_enabled()) {
        cxl_lit(&l, " ret=");
        cxl_hex(&l, r);
        cxl_end(&l);
    }
    ((void (*)(u32))mwf[F_MSG_DTOR])((u32)msg);
    return r;
}

static u32 inject(u32 obj, u32 mid, u32 pid, u32 value)
{
    struct iparam p;
    p.id = pid; p.type = 7; p.value = value;
    return inject_n(obj, mid, &p, 1);
}

/*
 * La cámara rechaza (ret=ffff) un mensaje si otro con el mismo MSGID sigue en
 * curso (bitácora v11d: dos IRIS_DRIVE_MODE seguidos, el segundo falló). Por eso
 * soltar el AE lock no va pegado a ponerlo: queda pendiente y se manda en la
 * siguiente llamada de la interfaz, al menos 80 ms después, reintentando hasta
 * que la cámara lo acepte. Después se vuelve a fijar el otro control.
 */
static volatile int pend_stage, pend_busy;
static u32 pend_mid, pend_pid, pend_val, pend_ms, pend_tries;

static u32 now_ms(void)
{
    long ts[2];
    sys3(263 /* clock_gettime */, 1 /* CLOCK_MONOTONIC */, (long)ts, 0);
    return (u32)ts[0] * 1000u + (u32)ts[1] / 1000000u;
}

/*
 * Paso de iris u obturación como pvt_SetIrisDrive / pvt_SetShutterDrive de
 * ObjCamera: ObjMsg(0x3100, SYNC_*_DRIVE) con un signed char, respuesta en un
 * ObjMsg vacío, IssueCommandSync al objeto de cámara.
 */
static u32 drive_sync(u32 obj, u32 mid, u32 pid, int step);
static u32 step_sync(u32 obj, int iris, int step)
{
    return drive_sync(obj, iris ? MSG_SYNC_IRIS_DRIVE : MSG_SYNC_SHUT_DRIVE, iris ? P_IRIS_DRIVE : P_SHUT_DRIVE, step);
}

static u32 drive_sync(u32 obj, u32 mid, u32 pid, int step)
{
    u32 msg[4] __attribute__((aligned(8)));
    u32 out[4] __attribute__((aligned(8)));
    u32 prm[4] __attribute__((aligned(8)));
    u32 r, i;
    if (!obj || !r_sync || !resolve_mwf()) return 0xffffffffu;
    for (i = 0; i < 4; i++) msg[i] = out[i] = prm[i] = 0;
    ((void (*)(u32, u32, u32))mwf[F_MSG_CTOR])((u32)msg, CAT_CAMERA, mid);
    ((void (*)(u32))mwf[F_MSG_CTOR0])((u32)out);
    ((void (*)(u32, u32))mwf[F_SCHAR_CTOR])((u32)prm, (u32)step);
    ((void (*)(u32, u32, u32))mwf[F_ADD])((u32)msg, pid, (u32)prm);
    ((void (*)(u32))mwf[F_SCHAR_DTOR])((u32)prm);
    n_rej = 0;
    our_call = 1;
    r = ((fn3_t)r_sync)(obj, (u32)msg, (u32)out);
    our_call = 0;
    if (r == V_INHIBITED && cx_autounlock && learn_unlock()) {
        n_rej = 0;
        our_call = 1;
        r = ((fn3_t)r_sync)(obj, (u32)msg, (u32)out);
        our_call = 0;
    }
    if (cx_log_enabled()) {
        struct line l;
        cxl_begin(&l, "ID");
        cxl_lit(&l, " m=");
        cxl_hex(&l, mid);
        cxl_lit(&l, " step=");
        cxl_hex(&l, (u32)step);
        cxl_lit(&l, " ret=");
        cxl_hex(&l, r);
        cxl_lit(&l, " out");
        walk((u32)out, &l);
        cxl_end(&l);
    }
    ((void (*)(u32))mwf[F_MSG_DTOR])((u32)out);
    ((void (*)(u32))mwf[F_MSG_DTOR])((u32)msg);
    return r;
}

/*
 * Valores reales que la cámara le manda a la interfaz (pvt_ProcessNotifyFNoEvt y
 * pvt_ProcessNotifyShutterSpeedEvt los leen con ObjMsg::GetParam), y corrección:
 * al volver a la prioridad de un control que estaba manual, la cámara lo pone
 * uno o dos pasos fuera (probado en la cámara: F2.8 -> F2.4, 1/180 -> 1/125).
 * Se recuerda el valor al salir de su prioridad y al volver se dan pasos hasta
 * que la cámara reporte el mismo.
 */
static volatile u32 v_fno, v_fno_min, v_sn, v_sd, v_seq[2];
static u32 t_fno, t_fno_min, t_sn, t_sd;
static volatile int t_ok[2];
struct fixer { int active, sign_known, stalls, steps, last; u32 t0, ts, seq, prev_a, prev_b; };
static struct fixer fx[2];      /* 0 iris, 1 obturación */
static int fx_sign[2] = { 1, 1 }; /* efecto de un paso +1: más F / más rápida (se corrige solo) */
static u32 r_getparam;
static int nf_logged;

/* 1 si el valor actual queda arriba del objetivo (F más cerrado / obturación más rápida), -1 abajo, 0 igual. */
static int cmp_cur(int k)
{
    if (k == 0) return v_fno > t_fno ? 1 : v_fno < t_fno ? -1 : 0;
    {
        u32 a = v_sn * t_sd, b = t_sn * v_sd;   /* tiempo actual vs objetivo, en cruz */
        return a < b ? 1 : a > b ? -1 : 0;
    }
}

static int cmp_prev(int k, u32 pa, u32 pb)
{
    if (k == 0) return v_fno > pa ? 1 : v_fno < pa ? -1 : 0;
    {
        u32 a = v_sn * pb, b = pa * v_sd;
        return a < b ? 1 : a > b ? -1 : 0;
    }
}

static void fx_log(const char *what, int k, int step)
{
    struct line l;
    if (!cx_log_enabled()) return;
    cxl_begin(&l, "FX");
    cxl_lit(&l, k ? " shutter " : " iris ");
    cxl_lit(&l, what);
    cxl_lit(&l, " cur=");
    if (k) { cxl_hex(&l, v_sn); cxl_lit(&l, "/"); cxl_hex(&l, v_sd); }
    else cxl_hex(&l, v_fno);
    cxl_lit(&l, " tgt=");
    if (k) { cxl_hex(&l, t_sn); cxl_lit(&l, "/"); cxl_hex(&l, t_sd); }
    else cxl_hex(&l, t_fno);
    cxl_lit(&l, " step=");
    cxl_hex(&l, (u32)step);
    cxl_lit(&l, " sign=");
    cxl_hex(&l, (u32)fx_sign[k]);
    cxl_end(&l);
}

static void note_value(u32 id, u32 v)
{
    struct line l;
    int k = (id == P_FNO) ? 0 : (id == P_SHUT_DENOM) ? 1 : -1;
    u32 old = id == P_FNO ? v_fno : id == P_FNO_CUR_MIN ? v_fno_min : id == P_SHUT_NUMER ? v_sn : v_sd;
    if (id == P_FNO) v_fno = v;
    else if (id == P_FNO_CUR_MIN) {
        v_fno_min = v;
        /* Llega después del FNO en la misma notificación: va con el objetivo. */
        if (cx_holdmanual && !fx[0].active && camera_mode == V_EXP_MODE_IRIS && cx_iris_held) t_fno_min = v;
        else if (fx[0].active && v != t_fno_min) {   /* cambió el zoom a media corrección */
            fx[0].active = 0;
            t_ok[0] = 0;
            fx_log("zoom cambió, dejo", 0, 0);
        }
    }
    else if (id == P_SHUT_NUMER) v_sn = v;
    else v_sd = v;
    if (k < 0) return;
    v_seq[k]++;
    /* En su propia prioridad y sin corregir, el valor de la cámara es el objetivo. */
    if (cx_holdmanual && !fx[k].active) {
        if (k == 0 && camera_mode == V_EXP_MODE_IRIS && cx_iris_held) { t_fno = v_fno; t_fno_min = v_fno_min; t_ok[0] = 1; }
        if (k == 1 && camera_mode == V_EXP_MODE_SHUTTER && cx_shutter_held && v_sd) { t_sn = v_sn; t_sd = v_sd; t_ok[1] = 1; }
    }
    if (cx_log_enabled() && (old != v || nf_logged < 4) && nf_logged < 300) {
        nf_logged++;
        cxl_begin(&l, "NF");
        if (k == 0) { cxl_lit(&l, " fno="); cxl_hex(&l, v_fno); cxl_lit(&l, " min="); cxl_hex(&l, v_fno_min); }
        else { cxl_lit(&l, " shut="); cxl_hex(&l, v_sn); cxl_lit(&l, "/"); cxl_hex(&l, v_sd); }
        cxl_lit(&l, " mode=");
        cxl_hex(&l, camera_mode);
        cxl_end(&l);
    }
}

/*
 * Bloqueos: InhFeatureCheck (libObj) pregunta con InhInfo::GetInhInfo si una
 * función está bloqueada (parámetro 0x1000, bool) antes de mandar la orden; si
 * lo está, IssueCommandAsync devuelve 0xa000 (bitácora v12: SET_ISO_SPEED_RATE
 * ret=a000). Con unlockiso, la función 0x116 (ISO) se contesta libre. La tabla
 * de bloqueos no se toca.
 */
#define SYM_GETINH "_ZN3MWF7InhInfo10GetInhInfoEN6DefInh11FeatureId_tEPNS_6ObjMsgE"
#ifndef CX_INH_LIB
#define CX_INH_LIB CX_MWF_LIB
#endif
#define FEAT_ISO 0x116
#define P_INH_FLAG 0x1000
static u32 r_getinh;

static u32 *find_param_type(u32 msg, u32 pid, u32 type)
{
    const u32 *body, *pm;
    u32 *chunk;
    u32 count, i, k = 0;
    if (!msg || (msg & 3)) return 0;
    body = (const u32 *)*(const u32 *)msg;
    if (!body || ((u32)body & 3)) return 0;
    pm = body + 3;
    count = pm[0];
    chunk = (u32 *)pm[1];
    if (count > 64) count = 64;
    for (i = 0; i < count && chunk && !((u32)chunk & 3); i++) {
        u32 *e = chunk + 1 + 3 * k;
        const u32 *t = (const u32 *)e[2];
        if (e[0] == pid && t && !((u32)t & 3) && t[0] == type) return &e[1];
        if (++k > 4) { k = 0; chunk = (u32 *)chunk[0]; }
    }
    return 0;
}

void cxw_getinh(u32 feature, u32 msg) __asm__(SYM_GETINH);
void cxw_getinh(u32 feature, u32 msg)
{
    static int logged;
    if (!r_getinh) {
        r_getinh = cx_dlsym(cx_lib_base(CX_INH_LIB), SYM_GETINH);
        if (!r_getinh) return;
    }
    ((void (*)(u32, u32))r_getinh)(feature, msg);
    if (our_call || n_unlocked) {
        u32 *f = find_param_type(msg, P_INH_FLAG, 1);
        int i;
        if (f && *f) {
            if (our_call && n_rej < NUNLOCK) rej_feat[n_rej++] = feature;
            for (i = 0; i < n_unlocked; i++)
                if (unlocked[i] == feature) { *f = 0; break; }
        }
    }
    if (cx_unlockiso && feature == FEAT_ISO) {
        u32 *f = find_param_type(msg, P_INH_FLAG, 1);
        if (f && *f) {
            *f = 0;
            if (cx_log_enabled() && logged < 20) {
                struct line l;
                logged++;
                cxl_begin(&l, "UL");
                cxl_lit(&l, " función 116 (ISO) contestada libre");
                cxl_end(&l);
            }
        }
    }
}

/* Valores de ISO y ganancia que lee la interfaz: sólo a la bitácora, al cambiar. */
static void note_iso(u32 msg, u32 id, u32 p)
{
    static u32 last[5], n;
    struct line l;
    u32 cat = 0, mid = 0, v = ((const u32 *)p)[0], ty = 0;
    const u32 *t = (const u32 *)((const u32 *)p)[1];
    int k = id == P_ISO_GAIN_STATUS ? 0 : id == P_ISO_GAIN_VALID ? 1 : id == P_ISO_SPEED_RATE ? 2 :
            id == P_ISO_SPEED_RATE_ACT ? 3 : 4;
    if (t && !((u32)t & 3)) ty = t[0];
    if (!cx_log_enabled() || n >= 200 || (last[k] == v + 1)) return;
    last[k] = v + 1;
    n++;
    msg_ids(msg, &cat, &mid);
    cxl_begin(&l, "NI");
    cxl_lit(&l, " m=");
    cxl_hex(&l, mid);
    cxl_lit(&l, " p");
    cxl_hex(&l, id);
    cxl_lit(&l, ":");
    cxl_hex(&l, ty);
    cxl_lit(&l, "=");
    cxl_hex(&l, v);
    cxl_end(&l);
}

#define SYM_GETPARAM "_ZNK3MWF6ObjMsg8GetParamEj"
u32 cxw_getparam(u32 msg, u32 id) __asm__(SYM_GETPARAM);
u32 cxw_getparam(u32 msg, u32 id)
{
    u32 p;
    if (!r_getparam) {
        r_getparam = cx_dlsym(cx_lib_base(CX_MWF_LIB), SYM_GETPARAM);
        if (!r_getparam) return 0;
    }
    p = ((fn2_t)r_getparam)(msg, id);
    if (p && !(p & 3) && lock_on && (id == P_EXPOSURE_BIAS_SET || id == P_EXPOSURE_BIAS_ACT)) {
        /* La pantalla ve su AE Shift, no el +N que va a la cámara. */
        static u32 fake[2][2] __attribute__((aligned(8)));
        const u32 *t = (const u32 *)((const u32 *)p)[1];
        if (t && !((u32)t & 3) && t[0] == 3) {
            int k = id == P_EXPOSURE_BIAS_SET ? 0 : 1;
            fake[k][0] = (u32)lock_k & 0xff;
            fake[k][1] = (u32)t;
            return (u32)fake[k];
        }
    }
    if (p && !(p & 3) && (id == P_ISO_GAIN_STATUS || id == P_ISO_GAIN_VALID || id == P_ISO_SPEED_RATE ||
                          id == P_ISO_SPEED_RATE_ACT || id == P_EXPOSURE_POS))
        note_iso(msg, id, p);
    if (p && !(p & 3) && (id == P_FNO || id == P_FNO_CUR_MIN || id == P_SHUT_NUMER || id == P_SHUT_DENOM)) {
        const u32 *t = (const u32 *)((const u32 *)p)[1];
        if (t && !((u32)t & 3) && t[0] == 6) note_value(id, ((const u32 *)p)[0] & 0xffff);
    }
    return p;
}

/* Arranca la corrección de un control que vuelve a su prioridad. */
static void fx_start(int k)
{
    if (!t_ok[k]) return;
    if (k == 0 && t_fno_min != v_fno_min) {   /* cambió el zoom: ese F ya no es el mismo iris */
        fx_log("zoom cambió, no corrijo", 0, 0);
        return;
    }
    fx[k].active = 1;
    fx[k].t0 = fx[k].ts = now_ms();
    fx[k].seq = v_seq[k];
    fx[k].stalls = fx[k].steps = fx[k].last = 0;
}

static void fx_run(int k, u32 t)
{
    struct fixer *f = &fx[k];
    int err, step;
    if (!f->active) return;
    if (t - f->t0 > CX_FIX_WINDOW_MS || f->steps >= 12) { f->active = 0; fx_log("fin por tiempo", k, 0); return; }
    if (f->last) {
        /* Esperar la respuesta del paso anterior. */
        if (f->seq == v_seq[k]) {
            if (t - f->ts < CX_FIX_WAIT_MS) return;
            if (++f->stalls >= 2) { f->active = 0; fx_log("sin respuesta, dejo", k, 0); return; }
        } else {
            int moved = cmp_prev(k, f->prev_a, f->prev_b);
            if (moved == 0) {
                if (++f->stalls >= 2) { f->active = 0; fx_log("no se mueve, dejo", k, 0); return; }
            } else if (moved != f->last * fx_sign[k]) {
                fx_sign[k] = -fx_sign[k];
                fx_log("signo al revés, corrijo", k, 0);
            }
        }
        f->last = 0;
    } else if (t - f->ts < CX_FIX_SETTLE_MS) {
        return;   /* dar tiempo a que llegue el valor tras el cambio de modo */
    }
    err = cmp_cur(k);
    if (err == 0) {
        /* Sigue atento un rato: la cámara a veces mueve el control unas décimas
         * de segundo después del cambio de prioridad (bitácora v16). */
        if (t - f->t0 < CX_FIX_ARMED_MS) return;
        f->active = 0;
        fx_log("listo", k, 0);
        return;
    }
    step = err > 0 ? -fx_sign[k] : fx_sign[k];
    f->prev_a = k ? v_sn : v_fno;
    f->prev_b = v_sd;
    f->seq = v_seq[k];
    f->ts = t;
    f->last = step;
    f->steps++;
    fx_log("paso", k, step);
    step_sync(camera_obj, k == 0, step);
}

void cxobj_tick(void)
{
    u32 r, t;
#ifdef CX_UX
    if (cx_uxlog && cx_log_enabled()) cxux_flush();
#endif
    if ((!pend_stage && !fx[0].active && !fx[1].active) || !camera_obj) return;
    if (__sync_lock_test_and_set(&pend_busy, 1)) return;
    t = now_ms();
    if (pend_stage && t - pend_ms >= CX_PEND_MS) {
        pend_ms = t;
        if (pend_stage == 1)
            r = inject(camera_obj, MSG_SET_AELOCK_MODE, P_AELOCK_MODE, V_AELOCK_OFF);
        else
            r = inject(camera_obj, pend_mid, pend_pid, pend_val);
        if (r == 0) pend_stage = pend_stage == 1 ? 2 : 0;
        else if (++pend_tries > 100 && pend_stage == 2) pend_stage = 0;
    }
    if (pend_stage != 1) {   /* con el AE lock puesto no se corrige */
        fx_run(0, t);
        fx_run(1, t);
    }
    __sync_lock_release(&pend_busy);
}

u32 cxw_async(u32 obj, u32 msg) __asm__(SYM_ASYNC);
u32 cxw_async(u32 obj, u32 msg)
{
    struct line l;
    u32 cat, mid, r = 0;
    resolve();
    cxobj_tick();
    if (cx_objmanual && msg_ids(msg, &cat, &mid) && cat == CAT_CAMERA && mid == MSG_SET_EXPOSURE_MODE) {
        u32 *v = find_param(msg, P_EXPOSURE_MODE);
        if (v && *v != V_EXP_MODE_IRIS && *v != V_EXP_MODE_SHUTTER && *v != V_EXP_MODE_MANUAL)
            cx_manual_active = 0;
        if (v && (*v == V_EXP_MODE_IRIS || *v == V_EXP_MODE_SHUTTER)) {
            cx_manual_active = 1;
            if (cx_log_enabled()) {
                cxl_begin(&l, "RW");
                cxl_lit(&l, " EXPOSURE_MODE ");
                cxl_hex(&l, *v);
                cxl_lit(&l, " -> ");
                cxl_hex(&l, V_EXP_MODE_MANUAL);
                cxl_end(&l);
            }
            *v = V_EXP_MODE_MANUAL;
        }
    }
    if (cx_log_enabled() && msg_ids(msg, &cat, &mid) && cat == CAT_CAMERA &&
        changed(mid, walk(msg, 0))) {
        head(&l, "OA", obj, cat, mid, msg);
        walk(msg, &l);
        cxl_end(&l);
    }
    {
        u32 newmode = 0, prev = 0;
        int aelocked = 0;
        int gain_db = -1;
        u32 iso = 0;
        int expo_turn = 0, expo_delta = 0;
        if (msg_ids(msg, &cat, &mid) && cat == CAT_CAMERA) camera_obj = obj;
        /* AE Shift como control de ganancia: cada paso fija el límite de ganancia. */
        if (cx_gainshift && msg_ids(msg, &cat, &mid) && cat == CAT_CAMERA && mid == MSG_SET_EXPOSURE_BIAS) {
            u32 *sw = find_param(msg, P_EXPOSURE_BIAS_SW), *b = find_param(msg, P_EXPOSURE_BIAS);
            if (sw && b) {
                int k = (int)*b;
                if (k < -4) k = -4;
                if (k > 4) k = 4;
                gain_db = *sw == V_EXPOSURE_BIAS_ON ? (k + 4) * 3 : -2;   /* -2: apagar el límite */
                /*
                 * gainlock: con iris y obturación fijos, el automático sólo mueve
                 * la ganancia; con el AE Shift en +N siempre quiere más luz y la
                 * sube hasta el tope. La pantalla sigue viendo su valor (GetParam).
                 */
                if (cx_gainlock && *sw == V_EXPOSURE_BIAS_ON) {
                    lock_k = k;
                    lock_on = 1;
                    *b = (u32)cx_gainlock;
                    if (cx_log_enabled()) {
                        cxl_begin(&l, "GL");
                        cxl_lit(&l, " AE Shift ");
                        cxl_hex(&l, (u32)k);
                        cxl_lit(&l, " -> camara +");
                        cxl_hex(&l, (u32)cx_gainlock);
                        cxl_end(&l);
                    }
                } else if (cx_gainlock) {
                    lock_on = 0;
                }
                /* El AE Shift real pasa intacto: si se apaga, la cámara contesta
                 * "Off" y la pantalla bloquea + y -. Con iris y obturación fijos,
                 * el desplazamiento sólo mueve la ganancia, y el límite la topa. */
            }
        }
        /* Prueba: AE Shift como exposición manual de fábrica (fija también la ganancia). */
        if (cx_expogain && msg_ids(msg, &cat, &mid) && cat == CAT_CAMERA && mid == MSG_SET_EXPOSURE_BIAS) {
            u32 *sw = find_param(msg, P_EXPOSURE_BIAS_SW), *b = find_param(msg, P_EXPOSURE_BIAS);
            if (sw && b) {
                int k = (int)*b;
                if (*sw == V_EXPOSURE_BIAS_ON) {
                    if (!expo_on) { expo_turn = 1; expo_k = k; }
                    else expo_delta = k - expo_k;
                    expo_k = k;
                } else if (expo_on) {
                    expo_turn = -1;
                }
            }
        }
        /* Prueba: AE Shift como ISO fijo, de 100 a 4000 en pasos de 2/3. */
        if (cx_isoshift && msg_ids(msg, &cat, &mid) && cat == CAT_CAMERA && mid == MSG_SET_EXPOSURE_BIAS) {
            u32 *sw = find_param(msg, P_EXPOSURE_BIAS_SW), *b = find_param(msg, P_EXPOSURE_BIAS);
            if (sw && b) {
                int k = (int)*b;
                if (k < -4) k = -4;
                if (k > 4) k = 4;
                iso = *sw == V_EXPOSURE_BIAS_ON ? V_ISO_100 + 2 * (u32)(k + 4) : V_ISO_AUTO;
            }
        }
        if (msg_ids(msg, &cat, &mid) && cat == CAT_CAMERA && mid == MSG_SET_EXPOSURE_MODE) {
            u32 *v = find_param(msg, P_EXPOSURE_MODE);
            if (v) {
                newmode = *v;
                prev = last_exp_mode;
                last_exp_mode = newmode;
                camera_obj = obj;
                /*
                 * Cambio de prioridad con el otro control manual. Probado en la
                 * cámara: si sólo se fija el otro control después del cambio,
                 * éste se mueve un instante (el iris brincó de F2.8 a F2.2); si
                 * se fija también antes, el control que entra vuelve a su valor
                 * de fábrica (1/60, F1.8). Aquí se bloquea la exposición (AE
                 * lock) durante el cambio y se fija el otro control después.
                 */
                if (cx_holdmanual && camera_mode != newmode && pend_stage != 1 &&
                    ((newmode == V_EXP_MODE_SHUTTER && cx_iris_held) ||
                     (newmode == V_EXP_MODE_IRIS && cx_shutter_held))) {
                    aelocked = 1;
                    inject(obj, MSG_SET_AELOCK_MODE, P_AELOCK_MODE, V_AELOCK_ON);
                }
                if (cx_holdmanual && camera_mode != newmode) {
                    /* El que sale de su prioridad deja de seguirse; el que vuelve se corrige. */
                    fx[0].active = fx[1].active = 0;
                    if (newmode == V_EXP_MODE_IRIS && cx_iris_held) fx_start(0);
                    if (newmode == V_EXP_MODE_SHUTTER && cx_shutter_held) fx_start(1);
                    /*
                     * El control que deja de ser la prioridad también se mueve
                     * un paso al cambiar (bitácora v16: F3.7 -> F3.4 al entrar a
                     * obturación, 1/1000 -> 1/725 al entrar a iris). Se corrige
                     * igual; si la cámara no acepta el paso fuera de su
                     * prioridad, fx_run lo deja tras dos intentos sin respuesta.
                     */
                    if (newmode == V_EXP_MODE_IRIS && cx_shutter_held) fx_start(1);
                    if (newmode == V_EXP_MODE_SHUTTER && cx_iris_held) fx_start(0);
                }
                camera_mode = *v;
            }
        }
        if (r_async) r = ((fn2_t)r_async)(obj, msg);
        /*
         * Lo que estaba manual se queda manual: tras cada prioridad (también si
         * se repite) se vuelve a mandar el otro control en manual. La cámara
         * pone el otro en automático con cada SET_EXPOSURE_MODE.
         */
        (void)prev;
        if (cx_holdmanual && newmode) {
            if (newmode == V_EXP_MODE_IRIS) {
                cx_iris_held = 1;
                if (cx_shutter_held)
                    inject(camera_obj, MSG_SET_SHUT_DRIVE_MODE, P_SHUT_DRIVE_MODE, V_SHUT_MANUAL);
            } else if (newmode == V_EXP_MODE_SHUTTER) {
                cx_shutter_held = 1;
                if (cx_iris_held)
                    inject(camera_obj, MSG_SET_IRIS_DRIVE_MODE, P_IRIS_DRIVE_MODE, V_IRIS_MANUAL);
            } else {
                cx_iris_held = cx_shutter_held = 0;
                t_ok[0] = t_ok[1] = 0;
                fx[0].active = fx[1].active = 0;
            }
            if (aelocked) {
                if (newmode == V_EXP_MODE_IRIS) {
                    pend_mid = MSG_SET_SHUT_DRIVE_MODE; pend_pid = P_SHUT_DRIVE_MODE; pend_val = V_SHUT_MANUAL;
                } else {
                    pend_mid = MSG_SET_IRIS_DRIVE_MODE; pend_pid = P_IRIS_DRIVE_MODE; pend_val = V_IRIS_MANUAL;
                }
                pend_ms = now_ms();
                pend_tries = 0;
                pend_stage = 1;
            }
        }
        if (expo_turn == 1) {
            inject(camera_obj, MSG_SET_MANUAL_EXPOSURE_MODE, P_MANUAL_EXPOSURE_MODE, V_MANUAL_EXP_ON);
            expo_on = 1;
        }
        while (expo_delta != 0) {
            int st = expo_delta > 0 ? 1 : -1;
            drive_sync(camera_obj, MSG_SYNC_MANUAL_EXPOSURE_DRIVE, P_MANUAL_EXPOSURE_DRIVE, st);
            expo_delta -= st;
        }
        if (expo_turn == -1) {
            inject(camera_obj, MSG_SET_MANUAL_EXPOSURE_MODE, P_MANUAL_EXPOSURE_MODE, V_MANUAL_EXP_OFF);
            expo_on = 0;
        }
        if (iso) {
            struct iparam p[3];
            p[0].id = P_ISO_SPEED_RATE;     p[0].type = 7; p[0].value = iso;
            p[1].id = P_ISO_SPEED_RATE_MIN; p[1].type = 7; p[1].value = iso == V_ISO_AUTO ? V_ISO_100 : iso;
            p[2].id = P_ISO_SPEED_RATE_MAX; p[2].type = 7; p[2].value = iso == V_ISO_AUTO ? V_ISO_6400 : iso;
            inject_n(camera_obj, MSG_SET_ISO_SPEED_RATE, p, 3);
        }
        if (gain_db != -1) {
            struct iparam p[2];
            p[0].id = P_AGC_LIMIT; p[0].type = 3; p[0].value = gain_db >= 0 ? (u32)gain_db : 0;
            p[1].id = P_AGC_LIMIT_MODE; p[1].type = 8; p[1].value = gain_db >= 0 ? V_AGC_LIMIT_ON : V_AGC_LIMIT_OFF;
            inject_n(camera_obj, MSG_SET_AGC_LIMIT, p, 2);
        }
    }
    return r;
}

u32 cxw_sync(u32 obj, u32 in, u32 out) __asm__(SYM_SYNC);
u32 cxw_sync(u32 obj, u32 in, u32 out)
{
    struct line l;
    u32 cat, mid, r = 0;
    int cam;
    resolve();
    if (msg_ids(in, &cat, &mid) && cat == CAT_CAMERA) {
        /* Un paso del usuario manda sobre la corrección. */
        if (mid == MSG_SYNC_IRIS_DRIVE) fx[0].active = 0;
        if (mid == MSG_SYNC_SHUT_DRIVE) fx[1].active = 0;
    }
    cam = cx_log_enabled() && msg_ids(in, &cat, &mid) && cat == CAT_CAMERA;
    if (r_sync) r = ((fn3_t)r_sync)(obj, in, out);
    if (cam && changed(0x80000000u | mid, walk(in, 0) ^ (walk(out, 0) * 31u) ^ r)) {
        head(&l, "OS", obj, cat, mid, in);
        walk(in, &l);
        cxl_lit(&l, " -> ret=");
        cxl_hex(&l, r);
        cxl_lit(&l, " out");
        walk(out, &l);
        cxl_end(&l);
    }
    return r;
}

void cxobj_drive(int iris, int manual)
{
    if (!manual) {   /* ese control pasa a automático: ya no hay valor que restaurar */
        t_ok[iris ? 0 : 1] = 0;
        fx[iris ? 0 : 1].active = 0;
    }
    if (!camera_obj) return;
    if (iris)
        inject(camera_obj, MSG_SET_IRIS_DRIVE_MODE, P_IRIS_DRIVE_MODE, manual ? V_IRIS_MANUAL : V_IRIS_AUTO);
    else
        inject(camera_obj, MSG_SET_SHUT_DRIVE_MODE, P_SHUT_DRIVE_MODE, manual ? V_SHUT_MANUAL : V_SHUT_AUTO);
}

void cxobj_start(void)
{
    resolve();
}
