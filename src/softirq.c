#include "hardirq.h"
#include "sched.h"

void irq_enter(void)
{
    __irq_enter_raw();
}

void irq_exit(void)
{
    __irq_exit_raw();
    if(current->need_resched == 1 && current->preempt_count == 0){
        schedule();
        current->need_resched = 0;
    }
}
