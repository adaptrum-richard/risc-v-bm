#include "sleep.h"
#include "wait.h"
#include "sched.h"
#include "proc.h"

DECLARE_WAIT_QUEUE_HEAD(sleep_queue);

void sleep(uint64 times)
{
    unsigned long timeout = jiffies + times*HZ;
    set_current_state(TASK_INTERRUPTIBLE);
    wait_event_interruptible(sleep_queue, timer_after_eq(jiffies, timeout));
}
static unsigned long prev_jiffies = 0;
void wakes_sleep(void)
{
    /*一秒钟唤醒一次在sleep中睡眠的进程*/
    if(0 == prev_jiffies){
        prev_jiffies = jiffies;
    } else {
        if(jiffies >= (prev_jiffies+HZ))
            wake_up(&sleep_queue);
    }
}
