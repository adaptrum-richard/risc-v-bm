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

void * const sys_call_table[__NR_syscalls] = {
    sys_printf,
    sys_sleep,
    sys_brk,
    sys_free,
    sys_ni_syscall
};
