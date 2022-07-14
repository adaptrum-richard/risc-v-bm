#include "sleep.h"
#include "wait.h"
#include "sched.h"
#include "proc.h"

DECLARE_WAIT_QUEUE_HEAD(sleep_queue);

void kernel_sleep(uint64 times)
{
    unsigned long timeout = jiffies + times*HZ;
    set_current_state(TASK_INTERRUPTIBLE);
    wait_event_interruptible(sleep_queue, timer_after_eq(jiffies, timeout));

}
static unsigned long timeout_period = 0;
void wakes_sleep(void)
{
    /*一秒钟唤醒一次在sleep中睡眠的进程*/
    if(0 == timeout_period){
        timeout_period = jiffies + HZ;
    } else {
        if(timer_after_eq(jiffies, timeout_period)){
            wake_up(&sleep_queue);
            timeout_period = jiffies + HZ;
        }
    }
}
