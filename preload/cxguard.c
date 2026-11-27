/*
 * cxguard: seguro de rescate para la HDR-CX580V, y punto de entrada de cxg.so.
 *
 * Se carga con LD_PRELOAD (vía /setting/env.txt) en todos los procesos que
 * lanza init. En bootin.elf, el primer proceso de la app, crea
 * /setting/updater/mode: el bootloader ve esa bandera en el siguiente
 * encendido y arranca el updater de nflasha1, desde donde el shell de PMCA
 * puede borrar estos archivos y dejar la cámara de fábrica.
 *
 * En im.elf (la app) arranca los módulos de diagnóstico (cxinh.c).
 *
 * Sin libc: sólo llamadas al kernel (ARM EABI), para no depender del cargador
 * más que en lo mínimo. Ningún error detiene al proceso.
 */

#include "cx.h"

#ifndef CX_FLAG_PATH
#define CX_FLAG_PATH "/setting/updater/mode"
#endif
#ifndef CX_APP_NAME
#define CX_APP_NAME "im.elf"
#endif

/* ¿argv[0] termina en name? */
static int argv0_is(const char *cmdline, long len, const char *name)
{
    long end = 0, nlen = 0, i;
    while (end < len && cmdline[end]) end++;
    while (name[nlen]) nlen++;
    if (end < nlen) return 0;
    if (end > nlen && cmdline[end - nlen - 1] != '/') return 0;
    for (i = 0; i < nlen; i++)
        if (cmdline[end - nlen + i] != name[i]) return 0;
    return 1;
}

static void cx_init(void)
{
    char buf[128];
    long fd, n;

    fd = cx_open("/proc/self/cmdline", O_RDONLY, 0);
    if (fd < 0) return;
    n = cx_read(fd, buf, sizeof(buf) - 1);
    cx_close(fd);
    if (n <= 0) return;
    buf[n] = 0;

    if (argv0_is(buf, n, "bootin.elf")) {
        fd = cx_open(CX_FLAG_PATH, O_WRONLY | O_CREAT, 0666);
        if (fd < 0) return;
        cx_fsync(fd);
        cx_close(fd);
    } else if (argv0_is(buf, n, CX_APP_NAME)) {
        cx_log_start();
        cxview_start();
        cxobj_start();
        cxinh_start();
    }
}

/* DT_INIT, el mecanismo más antiguo y el que todo cargador soporta. */
void _init(void) { cx_init(); }
