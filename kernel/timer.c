#include "timer.h"
#include "spinlock.h"
#include "list.h"
#include "jiffies.h"
#include "sched.h"
#include "types.h"
#include "printk.h"

struct timer_list timer_list_head = {
    .entry = LIST_HEAD_INIT(timer_list_head.entry),
};

void init_timer(struct timer_list *timer)
{
    initlock(&timer->lock, "timer");
    INIT_LIST_HEAD(&timer->entry);
    timer->function = NULL;
    timer->data = 0;
    timer->expires = 0;
    timer->magic = TIMER_MAGIC;
}

static void check_timer_failed(struct timer_list *timer)
{
    static int whine_count;
	if (whine_count < 16) {
		whine_count++;
		printk("Uninitialised timer!\n");
		printk("This is just a warning.  Your computer is OK\n");
		printk("function=0x%p, data=0x%lx\n",
			timer->function, timer->data);
	}
	/*
	 * Now fix it up
	 */
    initlock(&timer->lock, "timer");
	timer->magic = TIMER_MAGIC;    
}

static inline void check_timer(struct timer_list *timer)
{
    if (timer->magic != TIMER_MAGIC)
        check_timer_failed(timer);
}

int add_timer(struct timer_list *timer)
{
    unsigned long flags;
    unsigned long timeout = timer->expires;
    if(!timer)
        return -1;

    if(timer_after_eq(jiffies, timeout)){
        return -1;
    }
    check_timer(timer);
    spin_lock_irqsave(&timer_list_head.lock, flags);
    list_add_tail(&timer->entry, &timer_list_head.entry);
    spin_unlock_irqrestore(&timer_list_head.lock, flags);
    return 0;
}

int del_timer(struct timer_list *timer)
{
    unsigned long flags;
    if(!timer)
        return -1;
    check_timer(timer);
    spin_lock_irqsave(&timer_list_head.lock, flags);
    list_del(&timer->entry);
    spin_unlock_irqrestore(&timer_list_head.lock, flags);
    return 0;
}

int mod_timer(struct timer_list *timer, unsigned long expires)
{
    check_timer(timer);
    //acquire(&timer->lock);
    timer->expires = expires;
    //release(&timer->lock);
    return 0;
}

void run_timers(void)
{
    struct timer_list *timer;
    void (*func)(unsigned long) = NULL;
    unsigned long data;

    acquire(&timer_list_head.lock);
    if(list_empty(&timer_list_head.entry)){
        release(&timer_list_head.lock);
        return;
    }
    list_for_each_entry(timer, &timer_list_head.entry, entry){
        if(timer_after_eq(jiffies, timer->expires)){
            func = timer->function;
            data = timer->data;
            break;
        }
    }
    release(&timer_list_head.lock);
    acquire(&timer->lock);
    if(func){
        func(data);
    }
    release(&timer->lock);
}
