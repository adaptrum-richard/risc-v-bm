#ifndef __USER_H__
#define __USER_H__
#include "kernel/stat.h"
/*system call*/
extern void kprintf(char * buf);
extern void sleep(unsigned long);
extern unsigned long sbrk(unsigned long);
extern unsigned long brk(void *);
extern void free(void *);
extern int fork();
extern int wait(int *status);
extern void exit(int code);
extern int exec(char *path, char **argv);
extern int getpid();
//file system
extern unsigned long dup(int fd);
extern int open(const char *pathname, int omode);
extern unsigned long mknod(const char *pathname, short major, short minor);
extern int write(int fd, const void *buf, int count);
extern int close(int fd);
extern int read(int fd, void *buf, int count);
extern int pipe(int *fd);
extern int fstat(int fd, struct stat*);
extern int mkdir(const char*);
#endif
