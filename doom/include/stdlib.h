#ifndef CXD_STDLIB_H
#define CXD_STDLIB_H
#include <stddef.h>
#define EXIT_SUCCESS 0
#define EXIT_FAILURE 1
#define RAND_MAX 0x7fffffff
void *malloc(size_t);
void *calloc(size_t, size_t);
void *realloc(void *, size_t);
void free(void *);
int atoi(const char *);
long atol(const char *);
double atof(const char *);
long strtol(const char *, char **, int);
unsigned long strtoul(const char *, char **, int);
double strtod(const char *, char **);
int abs(int);
long labs(long);
void exit(int) __attribute__((noreturn));
void abort(void) __attribute__((noreturn));
char *getenv(const char *);
int system(const char *);
int rand(void);
void srand(unsigned);
int atexit(void (*)(void));
void qsort(void *, size_t, size_t, int (*)(const void *, const void *));
#endif
