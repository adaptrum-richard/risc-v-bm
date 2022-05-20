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

void wakes_sleep(void)
{
    wake_up(&sleep_queue);
}
