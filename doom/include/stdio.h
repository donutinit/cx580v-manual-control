/* Biblioteca C mínima para correr doomgeneric dentro de im.elf (ver doom/libc.c). */
#ifndef CXD_STDIO_H
#define CXD_STDIO_H
#include <stddef.h>
#include <stdarg.h>
typedef struct cxd_file FILE;
extern FILE *stdin, *stdout, *stderr;
#define EOF (-1)
#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2
int printf(const char *, ...);
int fprintf(FILE *, const char *, ...);
int sprintf(char *, const char *, ...);
int snprintf(char *, size_t, const char *, ...);
int vsnprintf(char *, size_t, const char *, va_list);
int vsprintf(char *, const char *, va_list);
int vfprintf(FILE *, const char *, va_list);
int vprintf(const char *, va_list);
int puts(const char *);
int putchar(int);
int fputs(const char *, FILE *);
int fputc(int, FILE *);
int putc(int, FILE *);
FILE *fopen(const char *, const char *);
int fclose(FILE *);
size_t fread(void *, size_t, size_t, FILE *);
size_t fwrite(const void *, size_t, size_t, FILE *);
int fseek(FILE *, long, int);
long ftell(FILE *);
int fflush(FILE *);
int feof(FILE *);
int ferror(FILE *);
int fgetc(FILE *);
int getc(FILE *);
char *fgets(char *, int, FILE *);
int remove(const char *);
int rename(const char *, const char *);
int sscanf(const char *, const char *, ...);
int fscanf(FILE *, const char *, ...);
void perror(const char *);
#endif
