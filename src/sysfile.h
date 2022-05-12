#ifndef __SYSFILE_H__
#define __SYSFILE_H__
#include "types.h"

uint64 __sys_close(int fd);
uint64 __sys_write(int fd, const void *buf, int count);
uint64 __sys_read(int fd, void *buf, int count);
uint64 __sys_open(const char *pathname, int omode);
int __sys_mknod(const char *pathname, short major, short minor);
#endif
