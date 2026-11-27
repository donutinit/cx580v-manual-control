/*
 * cxview: anota lo que la interfaz pide a la cámara, sin cambiar nada.
 *
 * cxg.so exporta cuatro funciones de ViewBase con el mismo nombre que las de
 * libObj.so. Como va precargada, el cargador resuelve hacia aquí las llamadas
 * de las pantallas (viewUnified*.so y la propia libObj.so). Cada envoltura
 * anota la llamada y pasa los mismos argumentos a la función original, que se
 * busca por nombre en la tabla dinámica de libObj.so ya cargada.
 *
 *   OV  ViewBase::openView(nombre, n)              abre una pantalla
 *   CV  ViewBase::closeView(nombre)                la cierra
 *   EX  ViewBase::requestModelExecute(modelo, acción, parámetros)
 *   GI  ViewBase::getModelInfo(modelo, consulta, salida, entrada)
 *       (sólo las 3 primeras veces por cada modelo y consulta)
 *
 * ParamList: puntero a un vector {inicio, fin, capacidad} de ParamBase*; cada
 * ParamBase es {vtable, tipo, clave, valor}.
 */

#include "cx.h"

#ifndef CX_VIEW_LIB
#define CX_VIEW_LIB "/libObj.so"
#endif
#ifndef CX_MODE_PATH
#define CX_MODE_PATH "/setting/cxmode.txt"
#endif

/*
 * Configuración de prueba en CX_MODE_PATH, una orden por línea:
 *
 *   iris | shutter | agc | aeshift | wbshift   Exposure abre esa pantalla (v4)
 *   redirect <origen> <destino>                 abrir <origen> abre <destino>
 *   aemode <n>                                  cuando una pantalla pida modo AE
 *                                               2 (iris) o 3 (obturación), se
 *                                               manda n; a la pantalla abierta se
 *                                               le reporta el modo que pidió
 *   objmanual 1                                 en la capa de cámara, mandar
 *                                               EXP_MODE_MANUAL en lugar de
 *                                               EXP_MODE_IRIS o EXP_MODE_SHUTTER
 *                                               (cxobj.c)
 *   holdmanual 1                                al pasar de obturación a iris (o
 *                                               al revés), mandar a la capa de
 *                                               cámara SHUT_DRIVE_MODE (o
 *                                               IRIS_DRIVE_MODE) manual (cxobj.c)
 *   gainshift 1                                 la pantalla AE Shift controla la
 *                                               ganancia: el paso k (-4..+4) pone
 *                                               el límite de ganancia en (k+4)*3 dB;
 *                                               el AE Shift real sigue activo
 *                                               (cxobj.c)
 *   isoshift 1                                  la pantalla AE Shift fija el ISO:
 *                                               el paso k (-4..+4) manda ISO 100,
 *                                               160, 250 ... 4000; apagado, ISO
 *                                               automático (cxobj.c, prueba)
 *   expogain 1                                  la pantalla AE Shift enciende la
 *                                               exposición manual de fábrica en la
 *                                               cámara y cada paso es un paso de
 *                                               exposición (cxobj.c, prueba)
 *   unlockiso 1                                 GetInhInfo contesta «libre» para la
 *                                               función 0x116 (ISO), bloqueada por
 *                                               el factor 0x1f7 (cxobj.c, prueba)
 *   autounlock 1                                si una orden propia de cxg.so sale
 *                                               bloqueada (0xa000), se contestan
 *                                               libres las funciones que la
 *                                               bloquearon y se reintenta una vez
 *   label <de> <a>                              la etiqueta de texto 0x0204<de> se
 *                                               muestra como la 0x0204<a> (cxtext.c)
 *   show <raíz> <id>, hide <raíz> <id>          muestra u oculta un ítem del menú de
 *                                               fábrica (cxmenu.c); raíz c347c = Camera/Mic
 *   image <de> <a>                              el icono 0x0220<de> se dibuja como
 *                                               el 0x0220<a> (cxtext.c)
 *   text <de> <texto>                           la etiqueta 0x0204<de> muestra un texto
 *                                               libre; '_' es espacio (cxtext.c, prueba)
 *   uxlog 1                                     sólo en cxg-ux.so: anota toques, botones
 *                                               y fuentes de etiquetas (cxux.c)
 *   textlog 1                                   anota los números de texto que pasan
 *   gainlock N                                  con gainshift: a la cámara le llega
 *                                               el AE Shift en +N (el automático
 *                                               sube la ganancia hasta el tope que
 *                                               elige la pantalla) y la pantalla
 *                                               sigue viendo su propio valor
 *   aeauto <n>                                  cuando una pantalla pida el modo
 *                                               automático de la PJ760V (5), se
 *                                               manda n (6 es el de fábrica de la
 *                                               CX580V)
 *
 * Origen y destino tienen que estar en las listas de abajo. Los nombres cortos
 * son los del menú; el motor los traduce a la implementación _L (FUN_007a3fec).
 */
static const char *const sources[] = {
    "view/EXPOSURE", "view/SPOT_METER", "view/SPOT_FOCUS", "view/SPOT_METER_FOCUS",
    "view/TELEMACRO", "view/FOCUS", "view/WHITE_BALANCE",
};
static const char *const targets[] = {
    "view/IRIS", "view/SHUTTER_SPEED", "view/AGC_LIMIT", "view/AE_SHIFT", "view/WB_SHIFT",
    "view/EXPOSURE",
};
static const struct { const char *mode, *view; } shortcuts[] = {
    { "iris",    "view/IRIS" },
    { "shutter", "view/SHUTTER_SPEED" },
    { "agc",     "view/AGC_LIMIT" },
    { "aeshift", "view/AE_SHIFT" },
    { "wbshift", "view/WB_SHIFT" },
};
#define NSRC (sizeof(sources) / sizeof(sources[0]))
#define NTGT (sizeof(targets) / sizeof(targets[0]))
#define MAXRD 8
static struct { const char *from, *to; } rd[MAXRD];
static int nrd;
static u32 aemode;                       /* 0: sin reescritura */
static u32 aeauto;                       /* 0: sin reescritura */
int cx_objmanual;
int cx_holdmanual;
int cx_gainshift;
int cx_expogain;
int cx_autounlock;
int cx_gainlock;
/* ¿Está abierta la pantalla de iris o la de obturación? Otras pantallas que se
 * abren encima (como view/CAUTION) no las cambian. */
static volatile int iris_open, shutter_open, last_manual;
static volatile u32 model_mode;   /* modo AE real del modelo (q=0xf, 0x12b), antes de corregirlo */

#define K_AEMODE_SET 0x12a               /* acción 0x10 */
#define K_AEMODE_Q1  0x12b               /* consulta 0xf */
#define K_AEMODE_Q2  0x12c
#define SYM_OPEN  "_ZN8ViewBase8openViewEPKci"
#define SYM_CLOSE "_ZN8ViewBase9closeViewEPKc"
#define SYM_EXEC  "_ZN8ViewBase19requestModelExecuteEPKcmP9ParamList"
#define SYM_INFO  "_ZN8ViewBase12getModelInfoEPKciP9ParamListPKS2_"

typedef u32 (*fn2_t)(u32, u32);
typedef u32 (*fn3_t)(u32, u32, u32);
typedef u32 (*fn4_t)(u32, u32, u32, u32);
typedef u32 (*fn5_t)(u32, u32, u32, u32, u32);

static u32 r_open, r_close, r_exec, r_info, lib_base;

static void resolve(void)
{
    if (r_exec && r_open && r_close && r_info) return;
    lib_base = cx_lib_base(CX_VIEW_LIB);
    r_open = cx_dlsym(lib_base, SYM_OPEN);
    r_close = cx_dlsym(lib_base, SYM_CLOSE);
    r_exec = cx_dlsym(lib_base, SYM_EXEC);
    r_info = cx_dlsym(lib_base, SYM_INFO);
}

static void log_plist(struct line *l, u32 plist)
{
    const u32 *pl = (const u32 *)plist, *vec;
    u32 b, e, i;
    if (!pl || (plist & 3)) return;
    vec = (const u32 *)pl[0];
    if (!vec || ((u32)vec & 3)) return;
    b = vec[0];
    e = vec[1];
    if (e < b || (b & 3)) return;
    for (i = 0; b + 4 * i < e && i < 10; i++) {
        const u32 *node = (const u32 *)*(const u32 *)(b + 4 * i);
        if (!node || ((u32)node & 3)) continue;
        cxl_str(l, " k", 2);
        cxl_hex(l, node[2]);
        cxl_str(l, ":", 1);
        cxl_hex(l, node[1]);
        cxl_str(l, "=", 1);
        cxl_hex(l, node[3]);
    }
}

static void header(void);

static void head(struct line *l, const char *kind, u32 self, u32 name)
{
    header();
    cxl_begin(l, kind);
    cxl_str(l, " ", 1);
    cxl_hex(l, self);
    cxl_lit(l, " \"");
    cxl_str(l, (const char *)name, 48);
    cxl_lit(l, "\"");
}

/* --- envolturas -------------------------------------------------------- */

static int same(const char *a, const char *b)
{
    while (*a && *a == *b) { a++; b++; }
    return *a == *b;
}

u32 cxw_open(u32 self, u32 name, u32 n) __asm__(SYM_OPEN);
u32 cxw_open(u32 self, u32 name, u32 n)
{
    struct line l;
    u32 r = 0;
    int i;
    resolve();
    cxobj_tick();
    for (i = 0; name && i < nrd; i++) {
        if (!same((const char *)name, rd[i].from)) continue;
        if (cx_log_enabled()) {
            head(&l, "RD", self, name);
            cxl_lit(&l, " -> ");
            cxl_str(&l, rd[i].to, 48);
            cxl_end(&l);
        }
        name = (u32)rd[i].to;
        break;
    }
    /* El menú de fábrica abre las pantallas por su nombre real, con _L. */
    if (name && (same((const char *)name, "view/IRIS") || same((const char *)name, "view/IRIS_L"))) {
        iris_open = 1;
        last_manual = 2;
    }
    if (name && (same((const char *)name, "view/SHUTTER_SPEED") || same((const char *)name, "view/SHUTTER_SPEED_L"))) {
        shutter_open = 1;
        last_manual = 3;
    }
    if (cx_log_enabled()) {
        head(&l, "OV", self, name);
        cxl_str(&l, " n=", 3);
        cxl_hex(&l, n);
        cxl_end(&l);
    }
    if (r_open) r = ((fn3_t)r_open)(self, name, n);
    cxpanel_view((const char *)name, 1);
    return r;
}

u32 cxw_close(u32 self, u32 name) __asm__(SYM_CLOSE);
u32 cxw_close(u32 self, u32 name)
{
    struct line l;
    u32 r = 0;
    resolve();
    cxobj_tick();
    if (name && (same((const char *)name, "view/IRIS") || same((const char *)name, "view/IRIS_L"))) iris_open = 0;
    if (name && (same((const char *)name, "view/SHUTTER_SPEED") || same((const char *)name, "view/SHUTTER_SPEED_L")))
        shutter_open = 0;
    if (cx_log_enabled()) {
        head(&l, "CV", self, name);
        cxl_end(&l);
    }
    if (r_close) r = ((fn2_t)r_close)(self, name);
    cxpanel_view((const char *)name, 0);
    return r;
}

/* Primer nodo de la lista con esa clave y tipo entero, o 0. */
static u32 *find_node(u32 plist, u32 key)
{
    const u32 *pl = (const u32 *)plist, *vec;
    u32 b, e, i;
    if (!pl || (plist & 3)) return 0;
    vec = (const u32 *)pl[0];
    if (!vec || ((u32)vec & 3)) return 0;
    b = vec[0];
    e = vec[1];
    if (e < b || (b & 3)) return 0;
    for (i = 0; b + 4 * i < e && i < 32; i++) {
        u32 *node = (u32 *)*(const u32 *)(b + 4 * i);
        if (node && !((u32)node & 3) && node[2] == key && node[1] == 1) return node;
    }
    return 0;
}

static int is_camera(u32 model) { return model && same((const char *)model, "model/CAMERA"); }

u32 cxw_exec(u32 self, u32 model, u32 action, u32 plist) __asm__(SYM_EXEC);
u32 cxw_exec(u32 self, u32 model, u32 action, u32 plist)
{
    struct line l;
    u32 r = 0;
    resolve();
    cxobj_tick();
    cxpanel_hook(2);
    /*
     * holdmanual, automático por control: "automático" en la pantalla de
     * obturación con el iris manual pasa a prioridad de iris (y al revés), en
     * vez de poner toda la exposición en automático.
     */
    if (cx_holdmanual && action == 0x10 && is_camera(model)) {
        u32 *n = find_node(plist, K_AEMODE_SET);
        /* Si el modelo ya está en ese modo no manda nada nuevo: el control
         * que se pidió en automático se pone en automático directamente. */
        if (n && n[3] == 5 && shutter_open && cx_iris_held) {
            cx_shutter_held = 0;
            n[3] = 2;
            if (cx_log_enabled()) { head(&l, "AU", self, model); cxl_lit(&l, " shutter auto, iris manual"); cxl_end(&l); }
            cxobj_drive(0, 0);
        } else if (n && n[3] == 5 && iris_open && cx_shutter_held) {
            cx_iris_held = 0;
            n[3] = 3;
            if (cx_log_enabled()) { head(&l, "AU", self, model); cxl_lit(&l, " iris auto, shutter manual"); cxl_end(&l); }
            cxobj_drive(1, 0);
        }
    }
    /*
     * holdmanual, primer toque: si la pantalla abierta es de un control manual
     * pero el modelo está en la otra prioridad, el primer paso se convierte en
     * "poner esta prioridad" (acción 0x10). El modelo cambia, la pantalla vuelve
     * a recibir su valor y el otro control sigue manual (cxobj.c lo reinyecta).
     */
    if (cx_holdmanual && is_camera(model) &&
        ((action == 0x2c && shutter_open && cx_shutter_held && model_mode != 3) ||
         (action == 0x2d && iris_open && cx_iris_held && model_mode != 2))) {
        u32 want = action == 0x2c ? 3 : 2;
        u32 *n = find_node(plist, action == 0x2c ? 0x1a8 : 0x1a7);
        if (n) {
            u32 key = n[2], val = n[3];
            if (cx_log_enabled()) {
                head(&l, "SY", self, model);
                cxl_lit(&l, " paso -> modo ");
                cxl_hex(&l, want);
                cxl_end(&l);
            }
            /* El modelo procesa la lista después, en su hilo: no se restaura.
             * La pantalla no la vuelve a usar; se la entrega al modelo. */
            (void)key; (void)val;
            n[2] = K_AEMODE_SET;
            n[3] = want;
            if (r_exec) r = ((fn4_t)r_exec)(self, model, 0x10, plist);
            model_mode = want;
            return r;
        }
    }
    if ((aemode || aeauto) && action == 0x10 && is_camera(model)) {
        u32 *n = find_node(plist, K_AEMODE_SET);
        if (aeauto && n && n[3] == 5) {
            if (cx_log_enabled()) {
                head(&l, "RW", self, model);
                cxl_lit(&l, " aeauto 5 -> ");
                cxl_hex(&l, aeauto);
                cxl_end(&l);
            }
            n[3] = aeauto;
        }
        if (aemode && n && (n[3] == 2 || n[3] == 3)) {
            if (cx_log_enabled()) {
                head(&l, "RW", self, model);
                cxl_lit(&l, " aemode ");
                cxl_hex(&l, n[3]);
                cxl_lit(&l, " -> ");
                cxl_hex(&l, aemode);
                cxl_end(&l);
            }
            n[3] = aemode;
        }
    }
    if (cx_log_enabled()) {
        head(&l, "EX", self, model);
        cxl_str(&l, " a=", 3);
        cxl_hex(&l, action);
        log_plist(&l, plist);
    }
    if (r_exec) r = ((fn4_t)r_exec)(self, model, action, plist);
    if (cx_log_enabled()) {
        cxl_str(&l, " ret=", 5);
        cxl_hex(&l, r);
        cxl_end(&l);
    }
    return r;
}

/*
 * getModelInfo se llama todo el tiempo. Se anota la primera respuesta de cada
 * (modelo, consulta) y después sólo cuando la respuesta cambia, hasta
 * MAX_CHANGES veces por consulta.
 */
#define SEEN 512
#define MAX_CHANGES 40
static struct { u32 name, id, sig, logged; } seen[SEEN];

static u32 out_sig(u32 plist, u32 ret)
{
    const u32 *pl = (const u32 *)plist, *vec;
    u32 b, e, i, h = 2166136261u ^ ret;
    if (!pl || (plist & 3)) return h;
    vec = (const u32 *)pl[0];
    if (!vec || ((u32)vec & 3)) return h;
    b = vec[0];
    e = vec[1];
    if (e < b || (b & 3)) return h;
    for (i = 0; b + 4 * i < e && i < 10; i++) {
        const u32 *node = (const u32 *)*(const u32 *)(b + 4 * i);
        if (!node || ((u32)node & 3)) continue;
        h = (h ^ node[2]) * 16777619u;
        h = (h ^ node[3]) * 16777619u;
    }
    return h;
}

static int should_log(u32 name, u32 id, u32 sig)
{
    u32 h = ((name >> 2) ^ (id * 2654435761u)) & (SEEN - 1), i;
    for (i = 0; i < SEEN; i++, h = (h + 1) & (SEEN - 1)) {
        if (seen[h].name == name && seen[h].id == id) {
            if (seen[h].sig == sig || seen[h].logged >= MAX_CHANGES) return 0;
            seen[h].sig = sig;
            seen[h].logged++;
            return 1;
        }
        if (!seen[h].name) {
            seen[h].name = name;
            seen[h].id = id;
            seen[h].sig = sig;
            seen[h].logged = 1;
            return 1;
        }
    }
    return 0;
}

u32 cxw_info(u32 self, u32 model, u32 id, u32 out, u32 in) __asm__(SYM_INFO);
u32 cxw_info(u32 self, u32 model, u32 id, u32 out, u32 in)
{
    struct line l;
    u32 r = 0;
    int logit;
    resolve();
    cxobj_tick();
    cxpanel_hook(1);
    if (r_info) r = ((fn5_t)r_info)(self, model, id, out, in);
    if (id == 0xf && is_camera(model)) {
        u32 *m = find_node(out, K_AEMODE_Q1);
        if (m) model_mode = m[3];
    }
    /*
     * La pantalla de iris sólo da pasos si el modo es 2, y la de obturación si
     * es 3. Con aemode, la cámara queda en otro modo; con objmanual, el modelo
     * no sabe traducir EXP_MODE_MANUAL y sigue diciendo 6. En los dos casos se
     * le responde a la pantalla abierta el modo que espera.
     */
    if ((aemode || (cx_objmanual && cx_manual_active)) && id == 0xf && is_camera(model) &&
        (iris_open || shutter_open)) {
        u32 want = !shutter_open ? 2 : !iris_open ? 3 : (u32)last_manual;
        u32 *a = find_node(out, K_AEMODE_Q1), *b = find_node(out, K_AEMODE_Q2);
        int force = cx_objmanual && cx_manual_active;
        if (a && (force || a[3] == aemode)) a[3] = want;
        if (b && (force || b[3] == aemode)) b[3] = want;
    }
    /*
     * Con holdmanual, el modelo sólo recuerda la última prioridad: al volver a
     * la otra pantalla diría "automático" aunque la cámara la tenga manual. Se
     * le responde a cada pantalla lo que de verdad está manual.
     */
    if (cx_holdmanual && id == 0xf && is_camera(model) &&
        ((iris_open && cx_iris_held) || (shutter_open && cx_shutter_held))) {
        u32 want = (iris_open && cx_iris_held && (!shutter_open || last_manual == 2)) ? 2 : 3;
        u32 *a = find_node(out, K_AEMODE_Q1), *b = find_node(out, K_AEMODE_Q2);
        if (a) a[3] = want;
        if (b) b[3] = want;
    }
    logit = cx_log_enabled() && is_camera(model) && should_log(model, id, out_sig(out, r));
    if (logit) {
        head(&l, "GI", self, model);
        cxl_str(&l, " q=", 3);
        cxl_hex(&l, id);
        if (in) {
            cxl_str(&l, " in", 3);
            log_plist(&l, in);
        }
        cxl_str(&l, " out", 4);
        log_plist(&l, out);
        cxl_str(&l, " ret=", 5);
        cxl_hex(&l, r);
        cxl_end(&l);
    }
    return r;
}

/*
 * El encabezado va con la primera llamada anotada, no al arrancar: si bootin
 * lanza varios im.elf, sólo el que tiene pantallas llega a escribir algo y es
 * el único que crea la bitácora.
 */
static void header(void)
{
    static volatile int done;
    struct line l;
    if (done || __sync_lock_test_and_set(&done, 1)) return;
    cxl_begin(&l, "HDR");
    cxl_str(&l, " base=", 6);
    cxl_hex(&l, lib_base);
    cxl_str(&l, " open=", 6);
    cxl_hex(&l, r_open - lib_base);
    cxl_str(&l, " close=", 7);
    cxl_hex(&l, r_close - lib_base);
    cxl_str(&l, " exec=", 6);
    cxl_hex(&l, r_exec - lib_base);
    cxl_str(&l, " info=", 6);
    cxl_hex(&l, r_info - lib_base);
    cxl_end(&l);
}

static const char *pick(const char *const *list, unsigned n, const char *w)
{
    unsigned i;
    for (i = 0; i < n; i++)
        if (same(w, list[i])) return list[i];
    return 0;
}

static void add_redirect(const char *from, const char *to)
{
    if (from && to && nrd < MAXRD) { rd[nrd].from = from; rd[nrd].to = to; nrd++; }
}

/* Hasta `max` dígitos hexadecimales; 0 si no lo es. */
static u32 parse_hex_n(const char *w, int max)
{
    u32 v = 0;
    int n = 0;
    for (; *w; w++, n++) {
        u32 d;
        if (*w >= '0' && *w <= '9') d = (u32)(*w - '0');
        else if (*w >= 'a' && *w <= 'f') d = (u32)(*w - 'a') + 10;
        else if (*w >= 'A' && *w <= 'F') d = (u32)(*w - 'A') + 10;
        else return 0;
        v = v * 16 + d;
    }
    return n >= 1 && n <= max ? v : 0;
}

static u32 parse_hex(const char *w) { return parse_hex_n(w, 4); }

static u32 parse_dec(const char *w)
{
    u32 v = 0;
    while (*w >= '0' && *w <= '9') v = v * 10 + (u32)(*w++ - '0');
    return *w ? 0 : v;
}

/* Lee CX_MODE_PATH (ver arriba). Lo que no entiende lo ignora. */
static void read_mode(void)
{
    static char m[1024];
    char *w[6];
    long fd = cx_open(CX_MODE_PATH, O_RDONLY, 0), n, i;
    unsigned k;
    int nw;
    if (fd < 0) return;
    n = cx_read(fd, m, sizeof(m) - 1);
    cx_close(fd);
    if (n <= 0) return;
    m[n] = 0;
    i = 0;
    while (i < n) {
        nw = 0;
        while (i < n && m[i] != '\n') {
            while (i < n && (m[i] == ' ' || m[i] == '\t' || m[i] == '\r')) m[i++] = 0;
            if (i >= n || m[i] == '\n') break;
            if (nw < 6) w[nw++] = &m[i];
            while (i < n && m[i] != ' ' && m[i] != '\t' && m[i] != '\r' && m[i] != '\n') i++;
        }
        if (i < n) m[i++] = 0;
        if (nw == 1) {
            for (k = 0; k < sizeof(shortcuts) / sizeof(shortcuts[0]); k++)
                if (same(w[0], shortcuts[k].mode)) add_redirect(sources[0], shortcuts[k].view);
        } else if (nw == 3 && same(w[0], "redirect")) {
            add_redirect(pick(sources, NSRC, w[1]), pick(targets, NTGT, w[2]));
        } else if (nw == 2 && same(w[0], "aemode")) {
            u32 v = parse_dec(w[1]);
            if (v >= 1 && v <= 6) aemode = v;
        } else if (nw == 2 && same(w[0], "gainshift")) {
            cx_gainshift = parse_dec(w[1]) == 1;
        } else if (nw == 2 && same(w[0], "expogain")) {
            cx_expogain = parse_dec(w[1]) == 1;
        } else if (nw == 2 && same(w[0], "autounlock")) {
            cx_autounlock = parse_dec(w[1]) == 1;
        } else if (nw == 3 && same(w[0], "label")) {
            cxtext_add(parse_hex(w[1]), parse_hex(w[2]));
        } else if (nw == 3 && (same(w[0], "show") || same(w[0], "hide"))) {
            cxmenu_add(parse_hex_n(w[1], 5), parse_dec(w[2]), same(w[0], "show"));
        } else if (nw == 3 && same(w[0], "image")) {
            cxtext_add_image(parse_hex(w[1]), parse_hex(w[2]));
        } else if (nw == 3 && same(w[0], "text")) {
            cxtext_add_text(parse_hex(w[1]), w[2]);
#ifdef CX_UX
        } else if (nw == 2 && same(w[0], "uxlog")) {
            cx_uxlog = parse_dec(w[1]) == 1;
#endif
        } else if (nw == 2 && same(w[0], "panel")) {
            u32 v = parse_dec(w[1]);
            if (v <= 2) cx_panel = (int)v;
            if (cx_panel) {               /* el panel necesita fijar el otro control y dar pasos destrabados */
                cx_holdmanual = 1;
                cx_autounlock = 1;
                if (!cx_gainlock) cx_gainlock = 4;
            }
        } else if (nw == 6 && same(w[0], "panelz")) {
            u32 k = parse_dec(w[1]), x = parse_dec(w[2]), y = parse_dec(w[3]), pw = parse_dec(w[4]), ph = parse_dec(w[5]);
            if (k < 7 && pw >= 16 && ph >= 16 && x + pw <= 640 && y + ph <= 480) {
                cx_panel_rect[k][0] = (int)x; cx_panel_rect[k][1] = (int)y;
                cx_panel_rect[k][2] = (int)pw; cx_panel_rect[k][3] = (int)ph;
            }
        } else if (nw == 5 && same(w[0], "hidebox")) {
            u32 x = parse_dec(w[1]), y = parse_dec(w[2]), bw = parse_dec(w[3]), bh = parse_dec(w[4]);
            /* La caja puede pasarse de la pantalla: hay botones nativos que miden un píxel de más. */
            if (bw >= 8 && bh >= 8 && x + bw <= 720 && y + bh <= 560) cxpanel_hidebox((int)x, (int)y, (int)bw, (int)bh);
        } else if (nw == 2 && same(w[0], "hidew")) {
            cxpanel_hideid(parse_hex_n(w[1], 8));
        } else if (nw == 2 && same(w[0], "hidevt")) {
            cxpanel_hidevt(parse_hex_n(w[1], 8));
        } else if (nw == 5 && same(w[0], "keepbox")) {
            u32 x = parse_dec(w[1]), y = parse_dec(w[2]), bw = parse_dec(w[3]), bh = parse_dec(w[4]);
            if (bw >= 4 && bh >= 4 && x + bw <= 640 && y + bh <= 480) cxpanel_keepbox((int)x, (int)y, (int)bw, (int)bh);
        } else if (nw == 2 && same(w[0], "paneldump")) {
            cx_panel_dump = parse_dec(w[1]) == 1;
        } else if (nw == 3 && same(w[0], "panelfont")) {
            u32 k = parse_dec(w[1]);
            if (k < 5) cx_panel_font[k] = parse_hex_n(w[2], 8);
        } else if (nw == 2 && same(w[0], "panelwb")) {
            u32 v = parse_dec(w[1]);
            if (v <= 2) cx_panel_wb = (int)v;
        } else if (nw == 2 && same(w[0], "paneldraw")) {
            cx_panel_draw = parse_dec(w[1]) == 1;
        } else if (nw == 2 && same(w[0], "textlog")) {
            cx_textlog = parse_dec(w[1]) == 1;
        } else if (nw == 2 && same(w[0], "gainlock")) {
            u32 v = parse_dec(w[1]);
            if (v <= 9) cx_gainlock = (int)v;
        } else if (nw == 2 && same(w[0], "holdmanual")) {
            cx_holdmanual = parse_dec(w[1]) == 1;
        } else if (nw == 2 && same(w[0], "objmanual")) {
            cx_objmanual = parse_dec(w[1]) == 1;
        } else if (nw == 2 && same(w[0], "aeauto")) {
            u32 v = parse_dec(w[1]);
            if (v >= 1 && v <= 6) aeauto = v;
        }
    }
}

void cxview_start(void)
{
    resolve();
    read_mode();
}
