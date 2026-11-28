/* Lo que comparten libc.c y cxd.c. */
#ifndef CXLIBC_H
#define CXLIBC_H
#include <stddef.h>
long cxd_sys(long n, long a, long b, long c, long d, long e, long f);
void cxd_heap_init(void *base, size_t size);
void cxd_out(const char *s, size_t n);          /* texto del motor, por líneas */
void cxd_log_line(const char *line);            /* una línea a la bitácora del anfitrión */
void cxd_exit(int code) __attribute__((noreturn));
int  cxd_setjmp(unsigned *buf);
void cxd_longjmp(unsigned *buf, int value) __attribute__((noreturn));
#endif
