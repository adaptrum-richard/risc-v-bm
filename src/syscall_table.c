#include "syscall.h"
#include "errorno.h"
#include "printk.h"
#include "sleep.h"

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

void * const sys_call_table[__NR_syscalls] = {
    sys_printf,
    sys_sleep,
    sys_ni_syscall
};
