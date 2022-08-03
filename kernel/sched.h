#ifndef __SCHED_H__
#define __SCHED_H__
#include "types.h"
#include "proc.h"
#include "jiffies.h"
#include "spinlock.h"

struct run_queue {
    struct spinlock lock;
    struct list_head rq_head;
    unsigned int nr_running;
    uint64 nr_switches;
    struct task_struct *curr;
};

struct sched_class {
    const struct sched_class *next;
    void (*task_fork)(struct task_struct *p);
    void (*enqueue_task)(struct run_queue *rq, struct task_struct *p);
    void (*dequeue_task)(struct run_queue *rq, struct task_struct *p);
    void (*task_tick)(struct run_queue *rq, struct task_struct *p);
    struct task_struct *(*pick_next_task)(struct run_queue *rq,
        struct task_struct *prev);
};

#define DEF_COUNTER (HZ/10)  /* 100ms time slice */
#define	MAX_SCHEDULE_TIMEOUT ((long)(~0UL>>1))

/*变量task，除了init_task*/
#define for_each_task(p)    \
    for(p = &init_task; (p = p->next_task) != &init_task;)

#define LINK_TASK(p) \
    do {        \
        LOCK_TASK_LIST();   \
        (p)->next_task = &init_task; \
        (p)->prev_task = init_task.prev_task; \
        init_task.prev_task->next_task = (p); \
        init_task.prev_task = (p); \
        UNLOCK_TASK_LIST(); \
    }while(0)

#define set_current_state(state_value)					\
    smp_store_mb(current->state, (state_value))		

void schedule_tail(struct task_struct *prev);
void schedule(void);
void timer_tick(void);
void set_task_sched_class(struct task_struct *p);
struct run_queue *cpu_rq(int cpu);
void sched_init(void);
void preempt_schedule_irq(void);
void traversing_rq(void);
long schedule_timeout(signed long timeout);
extern volatile uint64 jiffies;

#endif
