#ifndef __SYSFILE_H__
#define __SYSFILE_H__
#include "types.h"
#include "stat.h"

uint64 __sys_close(int fd);
uint64 __sys_write(int fd, const void *buf, int count);
uint64 __sys_read(int fd, void *buf, int count);
uint64 __sys_open(const char *pathname, int omode);
uint64 __sys_mknod(const char *pathname, short major, short minor);
uint64 __sys_mkdir(const char *pt);
uint64 __sys_fstat(int fd, struct stat *st);
uint64 __sys_dup(int oldfd);
uint64 __sys_chdir(const char *path);
uint64 __sys_pipe(int *fd);
uint64 __sys_unlink(const char *path);
uint64 __sys_link(const char *old, const char *new);
int fdalloc(struct file *f);
#endif
