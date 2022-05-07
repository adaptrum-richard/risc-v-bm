#include "fork.h"
#include "types.h"
#include "kalloc.h"
#include "proc.h"
#include "string.h"
#include "riscv.h"
#include "pt_regs.h"
#include "printk.h"
#include "sched.h"
#include "string.h"
#include "preempt.h"

extern void ret_from_kernel_thread(void);

static inline struct pt_regs *task_pt_regs(struct task_struct *tsk)
{
    unsigned long p = (uint64)tsk + THREAD_SIZE - sizeof(struct pt_regs);
    return (struct pt_regs *)p;
}

int copy_process(uint64 clone_flags, uint64 fn, uint64 arg, char *name)
{
    preempt_disable();
    struct task_struct *p = get_free_one_page();
    if(!p)
        return -1;
    memset(p, 0, sizeof(*p));
    struct pt_regs *childregs = task_pt_regs(p);
    memset(childregs, 0x0, sizeof(struct pt_regs));
    memset(&p->thread, 0x0, sizeof(p->thread));
    
    if(clone_flags & PF_KTHREAD){
        p->thread.s[0] = fn;
        p->thread.s[1] = arg;
        p->thread.ra = (uint64)ret_from_kernel_thread;
        p->state = RUNNING;
        p->flags = PF_KTHREAD;
        /* Supervisor irqs on: */
        childregs->status = SSTATUS_SPP | SSTATUS_SPIE;
    } else {
        printk("now , not implement user mode\n");
        preempt_enable();
        return -1;
    }
    
    p->priority = current->priority;
    p->counter = p->priority;
    p->preempt_count = 1;
    p->pid = nr_tasks++;
    if(!!name)
        strcpy(p->name, name);
    task[p->pid] = p;
    p->thread.sp = (uint64)childregs;
    preempt_enable();
    return 0;
}
