#ifndef __SCHED_H__
#define __SCHED_H__

void preempt_disable(void);
void preempt_enable(void);
void schedule_tail(void);
void schedule(void);
void timer_tick(void);

#endif
