#include "hardirq.h"
#include "sched.h"
#include "sleep.h"
#include "timer.h"

void irq_enter(void)
{
    __irq_enter_raw();
}

void irq_exit(void)
{
    __irq_exit_raw();
    /*只有在当前进程的preemt_count为0 且need_resched为1时才能被抢占调度*/
#if 0    
    if(READ_ONCE(current->preempt_count) != 0 )
        goto out;
    if(READ_ONCE(current->need_resched) == 1){
        preempt_schedule_irq();
    }

out:
#endif
    wakes_sleep();
    run_timers();
    return;
}
