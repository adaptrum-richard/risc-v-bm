#include "sleep.h"
#include "wait.h"
#include "sched.h"
#include "proc.h"

//DECLARE_WAIT_QUEUE_HEAD(sleep_queue);
struct wait_queue_head sleep_queue_array[NCPU];
void init_sleep_queue(void)
{
    for(int i = 0; i < NCPU; i++){
        sleep_queue_array[i].head.next = sleep_queue_array[i].head.prev =
            &sleep_queue_array[i].head;
        initlock(&sleep_queue_array[i].lock, "sleep_queue");
    }
}

void kernel_sleep(uint64 times)
{
    unsigned long timeout = jiffies + times*HZ;
    set_current_state(TASK_INTERRUPTIBLE);
    wait_event_interruptible(sleep_queue_array[smp_processor_id()], timer_after_eq(jiffies, timeout));

}
static unsigned long timeout_period = 0;
void wakes_sleep(void)
{
    /*一秒钟唤醒一次在sleep中睡眠的进程*/
    if(0 == timeout_period){
        timeout_period = jiffies + HZ;
    } else {
        if(timer_after_eq(jiffies, timeout_period)){
            wake_up(&sleep_queue_array[smp_processor_id()]);
            timeout_period = jiffies + HZ;
        }
    }
}
