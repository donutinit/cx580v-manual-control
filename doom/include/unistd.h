#ifndef CXD_UNISTD_H
#define CXD_UNISTD_H
#include <stddef.h>
#include <sys/types.h>
int close(int); ssize_t read(int, void *, size_t); ssize_t write(int, const void *, size_t);
off_t lseek(int, off_t, int); int access(const char *, int); int unlink(const char *); int isatty(int);
unsigned sleep(unsigned); int usleep(unsigned);
#define F_OK 0
#define R_OK 4
#endif
