#ifndef __SCHED_H__
#define __SCHED_H__
#include "types.h"
#include "proc.h"
#include "jiffies.h"
#include "spinlock.h"
#include "list.h"
#include "rbtree.h"

extern unsigned int     sched_min_granularity;
extern unsigned int     sched_latency;
extern const int        sched_prio_to_weight[40];
extern const uint32        sched_prio_to_wmult[40];


#define MAX_NICE	19
#define MIN_NICE	-20
#define NICE_WIDTH	(MAX_NICE - MIN_NICE + 1)

#define MAX_USER_RT_PRIO	100
#define MAX_RT_PRIO		MAX_USER_RT_PRIO

#define MAX_PRIO		(MAX_RT_PRIO + NICE_WIDTH)
#define DEFAULT_PRIO		(MAX_RT_PRIO + NICE_WIDTH / 2)
/*
 * Convert user-nice values [ -20 ... 0 ... 19 ]
 * to static priority [ MAX_RT_PRIO..MAX_PRIO-1 ],
 * and back.
 */
#define NICE_TO_PRIO(nice)	((nice) + DEFAULT_PRIO)
#define PRIO_TO_NICE(prio)	((prio) - DEFAULT_PRIO)

struct cfs_rq;
struct run_queue {
    struct spinlock lock;
    struct list_head rq_head;
    unsigned int nr_running;
    uint64 nr_switches;
    struct cfs_rq cfs;
    uint64 clock;
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
#define    MAX_SCHEDULE_TIMEOUT ((long)(~0UL>>1))

/*变量task，除了init_task*/
#define for_each_task(p, init_task)    \
    for(p = init_task; (p = p->next_task) != init_task;)

#define LINK_TASK(p, init_task) \
    do {        \
        LOCK_TASK_LIST();   \
        (p)->next_task = init_task; \
        (p)->prev_task = init_task->prev_task; \
        init_task->prev_task->next_task = (p); \
        init_task->prev_task = (p); \
        UNLOCK_TASK_LIST(); \
    }while(0)

#define set_current_state(state_value)                    \
    smp_store_mb(current->state, (state_value))        

#define task_on_runqueue(p) (p->run_list.next != NULL)

#define this_rq() get_cpu_rq()
void update_rq_clock(struct run_queue *rq);
void schedule_tail(struct task_struct *prev);
void schedule(void);
void timer_tick(void);
void set_task_sched_class(struct task_struct *p);
struct run_queue *cpu_rq(int cpu);
struct run_queue *task_rq(struct task_struct *p);
void sched_init(void);
void preempt_schedule_irq(void);
void traversing_rq(void);
long schedule_timeout(signed long timeout);
void wake_up_process(struct task_struct *p);
extern volatile uint64 jiffies;

/*sched fair*/
struct load_weight {
    unsigned long   weight;
    uint32    inv_weight;
};

struct cfs_rq;
struct sched_entity {
    /* For load-balancing: */
    struct load_weight  load;//调度实体的权重
    unsigned long       runnable_weight;//表示进程在可运行状态下的权重
    struct rb_node      run_node; //红黑树节点
    unsigned int        on_rq;//表示进程加入就绪队列，此值为1
    struct cfs_rq       *cfs_rq;
    uint64              exec_start;//计算调度实体虚拟时钟开始的时间
    uint64              sum_exec_runtime; //真实的总的运行时间
    uint64              vruntime;
    uint64              prev_sum_exec_runtime;//上一次真实的总的运行时间
};

/* CFS-related fields in a runqueue */
struct cfs_rq {
    struct load_weight    load; //就绪队列总的权重
    unsigned long        runnable_weight; //就是队列中可运行程序的总的权重
    unsigned int        nr_running; //可运行的进程的数量，而不是正在运行的进程数量
    unsigned int        h_nr_running;

    uint64            exec_clock; //统计就绪队列总的执行时间
    uint64            min_vruntime; //跟踪就绪队列中最小的虚拟时间的参考值，单步递增。

    struct rb_root_cached    tasks_timeline; //cfs红黑树的根

    /*
     * 'curr' points to currently running entity on this cfs_rq.
     * It is set to NULL otherwise (i.e when none are currently running).
     */
    struct sched_entity    *curr; //cfs调度器中正在执行的进程。
    struct sched_entity    *next;
    struct sched_entity    *last;//上一次执行的进程。
};


/*
 * Integer metrics need fixed point arithmetic, e.g., sched/fair
 * has a few: load, load_avg, util_avg, freq, and capacity.
 *
 * We define a basic fixed point arithmetic range, and then formalize
 * all these metrics based on that basic range.
 */
#define SCHED_FIXEDPOINT_SHIFT        10
#define SCHED_FIXEDPOINT_SCALE        (1L << SCHED_FIXEDPOINT_SHIFT)

#define NICE_0_LOAD_SHIFT    (SCHED_FIXEDPOINT_SHIFT + SCHED_FIXEDPOINT_SHIFT)
#define scale_load(w)        (w)
#define scale_load_down(w)   (w)
#define NICE_0_LOAD           (1L << NICE_0_LOAD_SHIFT)


#endif
