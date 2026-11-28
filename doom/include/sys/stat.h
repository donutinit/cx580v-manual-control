#ifndef CXD_SYS_STAT_H
#define CXD_SYS_STAT_H
#include <sys/types.h>
struct stat { mode_t st_mode; off_t st_size; };
#define S_IFMT 0170000
#define S_IFDIR 0040000
#define S_ISDIR(m) (((m) & S_IFMT) == S_IFDIR)
int stat(const char *, struct stat *);
int fstat(int, struct stat *);
int mkdir(const char *, mode_t);
#endif
