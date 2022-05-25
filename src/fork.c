#include "fork.h"
#include "types.h"
#include "page.h"
#include "proc.h"
#include "string.h"
#include "riscv.h"
#include "pt_regs.h"
#include "printk.h"
#include "sched.h"
#include "string.h"
#include "preempt.h"
#include "spinlock.h"
#include "barrier.h"

extern void ret_from_kernel_thread(void);

static inline struct pt_regs *task_pt_regs(struct task_struct *tsk)
{
    unsigned long p = (uint64)tsk + THREAD_SIZE - sizeof(struct pt_regs);
    return (struct pt_regs *)p;
}

int copy_process(uint64 clone_flags, uint64 fn, uint64 arg, char *name)
{
    int pid = -1;
    unsigned long flags;
    struct task_struct *p = (struct task_struct *)get_free_page();
    if(!p)
        return -1;
    
    if((pid = get_free_pid()) == -1){
        free_page((unsigned long)p);
        return -1;
    }
    memset(p, 0, sizeof(*p));
    struct pt_regs *childregs = task_pt_regs(p);
    memset(childregs, 0x0, sizeof(struct pt_regs));
    memset(&p->thread, 0x0, sizeof(p->thread));
    
    if(clone_flags & PF_KTHREAD){
        p->thread.s[0] = fn;
        p->thread.s[1] = arg;
        p->thread.ra = (uint64)ret_from_kernel_thread;
        p->state = TASK_RUNNING;
        p->flags = PF_KTHREAD | TASK_NORMAL;
        /* Supervisor irqs on: */
        childregs->status = SSTATUS_SPP | SSTATUS_SPIE;
    } else {
        printk("now , not implement user mode\n");
        return -1;
    }
    
    p->priority = current->priority;
    p->counter = DEF_COUNTER;
    p->preempt_count = 0;
    p->pid = pid;
    p->cpu = smp_processor_id();
    set_task_sched_class(p);
    if(name)
        strcpy(p->name, name);
    p->thread.sp = (uint64)childregs;
    LINK_TASK(p);
    mb();
    spin_lock_irqsave(&(cpu_rq(smp_processor_id())->lock),flags);
    p->sched_class->enqueue_task(cpu_rq(task_cpu(p)), p);
    spin_unlock_irqrestore(&(cpu_rq(smp_processor_id())->lock), flags);
    return 0;
}
