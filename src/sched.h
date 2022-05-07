#ifndef __SCHED_H__
#define __SCHED_H__
#include "types.h"

#define HZ 10

void schedule_tail(void);
void schedule(void);
void timer_tick(void);
void sleep(uint64 sec);
void wake(uint64 wait);
void wait(uint64 c);

extern volatile uint64 jiffies;

#endif
