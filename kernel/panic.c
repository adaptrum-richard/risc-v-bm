#include <stdarg.h>
#include "printk.h"
#include "proc.h"
#include "pt_regs.h"

extern void dump_backtrace(struct pt_regs *regs, struct task_struct *p);
void panic(const char *fmt, ...)
{
    va_list va;
    char buf[1024] = {0};
    char *ptr = buf;
    va_start(va, fmt);
    simple_vsprintf(&ptr, fmt, va);
    va_end(va);

    printk("Kernel panic: %s\n", buf);
    if (current->pid == 0)
        printk("In idle task - not syncing\n");

    dump_backtrace(NULL, current);
    while (1)
        ;
}
