#ifndef __TIMER_H__
#define __TIMER_H__
#include "list.h"
#include "spinlock.h"
/*实现定时器功能，超时后执行回调函数*/
#define TIMER_MAGIC	0x4b87ad6e
struct timer_list {
    struct list_head entry;
    unsigned long expires;
    struct spinlock lock;
    void (*function)(unsigned long);
    unsigned long data;
    unsigned int magic;
};

void run_timers(void);
int mod_timer(struct timer_list *timer, unsigned long expires);
int del_timer(struct timer_list *timer);
int add_timer(struct timer_list *timer);
void init_timer(struct timer_list *timer);

#endif
