#include "syscall.h"
#include "errorno.h"
#include "printk.h"
#include "sleep.h"
#include "mmap.h"
#include "sysfile.h"
#include "proc.h"
#include "sysproc.h"
#include "fork.h"
#include "printk.h"
#include "exec.h"
#include "exit.h"

long sys_ni_syscall(void)
{
    return -ENOSYS;
}

void sys_printf(char *buf)
{
    printk("%s", buf);

}

void sys_sleep(unsigned long s)
{
    kernel_sleep(s);
}

unsigned long sys_brk(unsigned long size)
{
    unsigned long addr = brk(size);
    return addr;
}

unsigned long sys_free(void *addr)
{
    return _free((unsigned long)addr);
}

unsigned long sys_fork(void)
{
    return do_fork();
}

void sys_exit(int code)
{
    do_exit(code);
}

unsigned long sys_wait(int *status)
{
    return do_wait(status);
}

unsigned long sys_pipe(void)
{
    return 0;
}

unsigned long sys_read(int fd, void *buf, int count)
{
    return __sys_read(fd, buf, count);
}

unsigned long sys_kill(void)
{
    return 0;
}

unsigned long sys_exec(char *path, char **argv)
{
    return exec(path, argv);
}

unsigned long sys_fstat(void)
{
    return 0;
}

unsigned long sys_chdir(const char *path)
{
    return __sys_chdir(path);
}

unsigned long sys_dup(int fd)
{
    return __sys_dup(fd);
}

unsigned long sys_getpid(void)
{
    return __sys_getpid();
}

unsigned long sys_uptime(void)
{
    return 0;
}

unsigned long sys_open(const char *pathname, int omode)
{
    return __sys_open(pathname, omode);
}

unsigned long sys_write(int fd, const void *buf, int count)
{
    return __sys_write(fd, buf, count);
}

unsigned long sys_mknod(const char *pathname, short major, short minor)
{
    return __sys_mknod(pathname, major, minor);
}

unsigned long sys_unlink(void)
{
    return 0;
}

unsigned long sys_link(void)
{
    return 0;
}

unsigned long sys_mkdir(const char *path)
{
    return __sys_mkdir(path);
}

unsigned long sys_close(int fd)
{
    return __sys_close(fd);
}

void * const sys_call_table[__NR_syscalls] = {
    sys_printf, //0
    sys_fork,   //1
    sys_exit,   //2
    sys_wait,   //3
    sys_pipe,   //4
    sys_read,   //5
    sys_kill,   //6
    sys_exec,   //7
    sys_fstat,  //8
    sys_chdir,  //9
    sys_dup,    //10
    sys_getpid, //11
    sys_brk,    //12
    sys_sleep,  //13
    sys_uptime, //14
    sys_open,   //15
    sys_write,  //16
    sys_mknod,  //17
    sys_unlink, //18
    sys_link,   //19
    sys_mkdir,  //20
    sys_close,  //21
    sys_free,   //22
    sys_ni_syscall, //23
};
