#include "syscall.h"
#include "errorno.h"
#include "printk.h"
#include "sleep.h"
#include "mmap.h"

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
    sleep(s);
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
    return 0;
}

unsigned long sys_exit(void)
{
    return 0;
}

unsigned long sys_wait(void)
{
    return 0;
}

unsigned long sys_pipe(void)
{
    return 0;
}

unsigned long sys_read(void)
{
    return 0;
}

unsigned long sys_kill(void)
{
    return 0;
}

unsigned long sys_exec(void)
{
    return 0;
}

unsigned long sys_fstat(void)
{
    return 0;
}

unsigned long sys_chdir(void)
{
    return 0;
}

unsigned long sys_dup(void)
{
    return 0;
}

unsigned long sys_getpid(void)
{
    return 0;
}

unsigned long sys_uptime(void)
{
    return 0;
}

unsigned long sys_open(void)
{
    return 0;
}

unsigned long sys_write(void)
{
    return 0;
}

unsigned long sys_mknod(void)
{
    return 0;
}

unsigned long sys_unlink(void)
{
    return 0;
}

unsigned long sys_link(void)
{
    return 0;
}

unsigned long sys_mkdir(void)
{
    return 0;
}

unsigned long sys_close(void)
{
    return 0;
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
