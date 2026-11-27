/*
 * Textos de la interfaz: cambia una etiqueta por otra que ya existe en la tabla
 * de textos de la cámara, por número y sin tocar el firmware.
 *
 * Los textos están en /usr/share/app/string_<idioma>.uxc y se piden por un
 * número de 32 bits, 0x0204xxxx, igual en todos los idiomas (0x02042614 es
 * «Exposure», 0x020428b6 «IRIS», 0x020426f0 «Gain»...). Las etiquetas llegan a
 * la pantalla por ux::gfxeng::TextSprite::setString(unsigned int) (lo usa
 * GEN_Label::setText) y, en menor medida, por
 * ux::resmgr::ResourceManagerImpl::getText(unsigned int, UString&). Las dos
 * están en libObj.so, se llaman por la PLT y son Thumb.
 *
 * Los iconos van igual: números 0x0220xxxx de images/image_*.uxc (nflasha7) que
 * GEN_Icon::setImage(unsigned int, bool) recibe por la PLT. La pantalla AE Shift
 * dibuja su valor con iconos de 72x64 (0x0220001c..25) y el firmware trae, del
 * mismo tamaño, los de «0dB»...«24dB» y «AGC OFF» (0x02200187..9b).
 *
 * cxmode.txt:  label <de> <a>    los 16 bits bajos de cada número, en hexadecimal
 *              image <de> <a>    lo mismo para iconos
 *              text <de> <texto> la etiqueta muestra un texto libre ('_' = espacio),
 *                                con TextSprite::setString(char const*). Prueba.
 *              textlog 1         anota cada número distinto que pasa (inventario)
 */
#include "cx.h"

#ifndef CX_TEXT_LIB
#define CX_TEXT_LIB "/libObj.so"
#endif
#define SYM_SETSTR  "_ZN2ux6gfxeng10TextSprite9setStringEj"
#define SYM_GETTEXT "_ZN2ux6resmgr19ResourceManagerImpl7getTextEjRNS_4core7UStringE"
#define SYM_SETIMAGE "_ZN8GEN_Icon8setImageEjb"
#define SYM_SETSTR_C "_ZN2ux6gfxeng10TextSprite9setStringEPKc"
#define NTEXT       4
#define TEXT_MAX    24
#define TEXT_NS     0x0204u          /* bits 16..25 del número */
#define IMAGE_NS    0x0220u
#define NLABEL      24
#define NIMAGE      16
#define MAX_TEXTLOG 150

struct rule { u32 from, to; int logged; };
static struct rule lab[NLABEL], img[NIMAGE];
static int nlab, nimg;
int cx_textlog;
static u32 r_setstr, r_gettext, r_setimage, r_setstr_c;
static struct { u32 from; char s[TEXT_MAX]; int logged; } txt[NTEXT];
static int ntxt;

void cxtext_add(u32 from, u32 to)
{
    if (!from || !to || from > 0xffff || to > 0xffff || nlab >= NLABEL) return;
    lab[nlab].from = from;
    lab[nlab].to = to;
    nlab++;
}

void cxtext_add_image(u32 from, u32 to)
{
    if (!from || !to || from > 0xffff || to > 0xffff || nimg >= NIMAGE) return;
    img[nimg].from = from;
    img[nimg].to = to;
    nimg++;
}

void cxtext_add_text(u32 from, const char *word)
{
    int i;
    if (!from || from > 0xffff || !word || !word[0] || ntxt >= NTEXT) return;
    for (i = 0; i < TEXT_MAX - 1 && word[i]; i++) txt[ntxt].s[i] = word[i] == '_' ? ' ' : word[i];
    txt[ntxt].s[i] = 0;
    txt[ntxt].from = from;
    ntxt++;
}

static void resolve(void)
{
    static int tries;
    u32 base;
    if ((r_setstr && r_gettext && r_setimage) || tries >= 3) return;   /* estas funciones se llaman muchísimo */
    tries++;
    base = cx_lib_base(CX_TEXT_LIB);
    r_setstr = cx_dlsym(base, SYM_SETSTR);
    r_gettext = cx_dlsym(base, SYM_GETTEXT);
    r_setimage = cx_dlsym(base, SYM_SETIMAGE);
    r_setstr_c = cx_dlsym(base, SYM_SETSTR_C);
}

static void note(u32 id)
{
    static u32 seen[65536 / 32];
    static int n;
    struct line l;
    u32 lo = id & 0xffff;
    if (n >= MAX_TEXTLOG || (seen[lo >> 5] & (1u << (lo & 31)))) return;
    seen[lo >> 5] |= 1u << (lo & 31);
    n++;
    cxl_begin(&l, "TS");
    cxl_lit(&l, " ");
    cxl_hex(&l, id);
    cxl_end(&l);
}

/* El número que hay que pedir en lugar de id. `via` sólo va a la bitácora. */
static u32 apply(struct rule *r, int n, u32 ns, u32 id, const char *kind, const char *via)
{
    struct line l;
    int i;
    if (((id >> 16) & 0x3ff) != ns) return id;
    for (i = 0; i < n; i++) {
        if (r[i].from != (id & 0xffff)) continue;
        if (r[i].logged < 2 && cx_log_enabled()) {
            r[i].logged++;
            cxl_begin(&l, kind);
            cxl_lit(&l, via);
            cxl_hex(&l, id);
            cxl_lit(&l, " -> ");
            cxl_hex(&l, r[i].to);
            cxl_end(&l);
        }
        return (id & 0xffff0000u) | r[i].to;
    }
    return id;
}

static u32 remap(u32 id, const char *via)
{
    if (cx_textlog && cx_log_enabled() && ((id >> 16) & 0x3ff) == TEXT_NS) note(id);
    return apply(lab, nlab, TEXT_NS, id, "TX", via);
}

void cxw_setstr(u32 self, u32 id) __asm__(SYM_SETSTR);
void cxw_setstr(u32 self, u32 id)
{
    struct line l;
    int i;
    resolve();
#ifdef CX_UX
    cxux_note_label(self, id);
#endif
    for (i = 0; i < ntxt && r_setstr_c; i++) {
        if (((id >> 16) & 0x3ff) != TEXT_NS || txt[i].from != (id & 0xffff)) continue;
        if (txt[i].logged < 2 && cx_log_enabled()) {
            txt[i].logged++;
            cxl_begin(&l, "TX");
            cxl_lit(&l, " free text for ");
            cxl_hex(&l, id);
            cxl_end(&l);
        }
        ((void (*)(u32, const char *))r_setstr_c)(self, txt[i].s);
        return;
    }
    if (r_setstr) ((void (*)(u32, u32))r_setstr)(self, remap(id, " sprite "));
}

u32 cxw_gettext(u32 self, u32 id, u32 out) __asm__(SYM_GETTEXT);
u32 cxw_gettext(u32 self, u32 id, u32 out)
{
    resolve();
    if (!r_gettext) return 0;
    return ((u32 (*)(u32, u32, u32))r_gettext)(self, remap(id, " text "), out);
}

void cxw_setimage(u32 self, u32 id, u32 flag) __asm__(SYM_SETIMAGE);
void cxw_setimage(u32 self, u32 id, u32 flag)
{
    resolve();
    if (r_setimage) ((void (*)(u32, u32, u32))r_setimage)(self, apply(img, nimg, IMAGE_NS, id, "TI", " icon "), flag);
}

void cxtext_start(void)
{
    resolve();
}
