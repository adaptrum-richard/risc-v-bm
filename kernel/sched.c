#include "sched.h"
#include "proc.h"
#include "riscv.h"
#include "printk.h"
#include "preempt.h"
#include "jiffies.h"
#include "wait.h"
#include "spinlock.h"
#include "vm.h"
#include "mm.h"
#include "timer.h"

extern struct sched_class simple_sched_class;
extern struct sched_class fair_sched_class;
volatile uint64 jiffies = 0;
struct run_queue g_rq[NCPU];

//一个进程最少要执行0.75毫秒, sched_min_granularity单位是纳秒
unsigned int sched_min_granularity = 750000ULL;
//6ms ,单位是纳秒
unsigned int sched_latency = 6000000ULL;


/*
 *不同的nice值对于数组中的一个值，每个值之间相差10%的cpu使用率
 */
const int sched_prio_to_weight[40] = {
 /* -20 */     88761,     71755,     56483,     46273,     36291,
 /* -15 */     29154,     23254,     18705,     14949,     11916,
 /* -10 */      9548,      7620,      6100,      4904,      3906,
 /*  -5 */      3121,      2501,      1991,      1586,      1277,
 /*   0 */      1024,       820,       655,       526,       423,
 /*   5 */       335,       272,       215,       172,       137,
 /*  10 */       110,        87,        70,        56,        45,
 /*  15 */        36,        29,        23,        18,        15,
};

/*
 * vruntime= (delta_exec*nice_0_weight * 2^32) >> 32  (1)
 * inv_weight = 2^32 / weight    (2)
 *  sched_prio_to_wmult 即公式(2)的预设值
 */
const uint32 sched_prio_to_wmult[40] = {
 /* -20 */     48388,     59856,     76040,     92818,    118348,
 /* -15 */    147320,    184698,    229616,    287308,    360437,
 /* -10 */    449829,    563644,    704093,    875809,   1099582,
 /*  -5 */   1376151,   1717300,   2157191,   2708050,   3363326,
 /*   0 */   4194304,   5237765,   6557202,   8165337,  10153587,
 /*   5 */  12820798,  15790321,  19976592,  24970740,  31350126,
 /*  10 */  39045157,  49367440,  61356676,  76695844,  95443717,
 /*  15 */ 119304647, 148102320, 186737708, 238609294, 286331153,
};

static inline struct run_queue *get_cpu_rq()
{
    return &g_rq[smp_processor_id()];
}

struct run_queue *task_rq(struct task_struct *p)
{
    return &g_rq[p->cpu];
}

unsigned long sched_clock_cpu(int cpu)
{
    return read_mtime() * TIMER_MS_TO_NS;
}

void update_rq_clock(struct run_queue *rq)
{
    long delta;
    delta = sched_clock_cpu(smp_processor_id()) - rq->clock;
    if(delta < 0)
        return;
    /*TODO: 如果这里发生了中断，时间会不准确，如何保证这里时间准确？*/
    rq->clock += delta;
}

void traversing_rq(void)
{
    struct run_queue *rq = get_cpu_rq();
    unsigned long flags;
    struct list_head *tmp;
    struct task_struct *p;
    spin_lock_irqsave(&rq->lock, flags);
    list_for_each(tmp, &rq->rq_head) {
        p = list_entry(tmp, struct task_struct, run_list);
        printk("hart: %d, pid: %d, name: %s, counter: %d, preempt: 0x%x, need_resched: %lu\n", 
            smp_processor_id(), p->pid, p->name, p->counter, p->preempt_count, p->need_resched);
    }
    spin_unlock_irqrestore(&rq->lock, flags);
}

extern struct task_struct *cpu_switch_to(struct thread_struct *prev, struct thread_struct *next);

void schedule_tail(struct task_struct *prev)
{
    intr_on();
}
extern void set_pgd(uint64);
struct task_struct *switch_to(struct task_struct *prev, struct task_struct *next)
{
    uint64 pgd = 0; 
    if(current == next){
        return NULL;
    }
    w_current((uint64)next);
    if(next->mm && next->mm->pagetable){
        pgd = MAKE_SATP(next->mm->pagetable);
    }
    set_pgd(pgd);
    return cpu_switch_to(&prev->thread, &next->thread);
}

/* 检查是否在中断上下文里发生了调度，这是一个不好
* 的习惯。因为中断上下文通常是关中断的，若发生
* 调度了，CPU选择了next进程运行，CPU就运行在
* next进程中，那么可能长时间处于关中断状态，这样
* 时钟tick有可能丢失，导致系统卡住了
*/
static void schedule_debug(struct task_struct *p)
{
    if(in_atomic_preempt_off()) { 
        printk("BUG: schedule while atomic pid:%d, preempt_count:0x%x, "
            "task_name = %s, cpu = %d, task addr = 0x%lx\n", 
            p->pid, preempt_count(), current->name, smp_processor_id(), current);
        panic("schedule_debug");
    }
}

void sched_init(void)
{
    struct run_queue *rq = get_cpu_rq();
    INIT_LIST_HEAD(&rq->rq_head);
    initlock(&rq->lock, "rqlock");
    rq->nr_running = 0;
    rq->nr_switches = 0;
    rq->curr = NULL;
}

struct task_struct *pick_next_task(struct run_queue *rq,
    struct task_struct *prev)
{
    const struct sched_class *class = &simple_sched_class;
    return class->pick_next_task(rq, prev);
}

void dequeue_task(struct run_queue *rq, struct task_struct *p)
{
    const struct sched_class *class = p->sched_class;
    class->dequeue_task(rq, p);
}

void enqueue_task(struct run_queue *rq, struct task_struct *p)
{
    p->sched_class->enqueue_task(rq, p);
}

void task_tick(struct run_queue *rq, struct task_struct *p)
{
    const struct sched_class *class = p->sched_class;
    return class->task_tick(rq, p);
}

void set_task_sched_class(struct task_struct *p)
{
    if(p->prio <= MAX_USER_RT_PRIO) 
        p->sched_class = &simple_sched_class;
    else
        p->sched_class = &fair_sched_class;
}

void tick_handle_periodic(void)
{
    struct run_queue *rq = get_cpu_rq();
    task_tick(rq, current);
}

static void clear_task_resched(struct task_struct *p)
{
    p->need_resched = 0;
}

static void __schedule(void)
{
    struct task_struct *prev, *next, *last;
    struct run_queue *rq = get_cpu_rq();
    prev = current;
    /* 检查是否在中断上下文中发生了调度 */
    schedule_debug(prev);
    
    /*关闭中断，以免发送影响调度器*/
    intr_off();
    __smp_rmb();
    if(prev->state != TASK_RUNNING)
        dequeue_task(rq, prev);

    next = pick_next_task(rq, prev);
    clear_task_resched(prev);
    if(next != prev){
        last = switch_to(prev, next);
        rq->nr_switches++;
        rq->curr = current;
    }
    schedule_tail(last);
}

/*普通调度*/
void schedule(void)
{
    preempt_disable();
    __schedule();
    preempt_enable();
}

/*抢占调度*/
void preempt_schedule_irq(void)
{
    preempt_disable();
    intr_on();
    __schedule();
    intr_off();
    preempt_enable();
}

void wake_up_process(struct task_struct *p)
{
    unsigned long flags;
    struct run_queue *rq = get_cpu_rq();
    spin_lock_irqsave(&rq->lock, flags);
    p->state = TASK_RUNNING;
    if(!task_on_runqueue(p))
        enqueue_task(rq, p);
    spin_unlock_irqrestore(&rq->lock, flags);
}

void timer_tick(void)
{
    tick_handle_periodic();
    if(smp_processor_id() == 0){
        jiffies++;
    }
}

struct run_queue *cpu_rq(int cpu)
{
    if(cpu >= NCPU){
        panic("%s %d\n", __func__, __LINE__);
    }
    return &g_rq[cpu];
}

static inline void __enqueue_task(struct run_queue *rq, struct task_struct *p)
{
    p->sched_class->enqueue_task(rq, p);
}

void activate_task(struct run_queue *rq, struct task_struct *p)
{
    __enqueue_task(rq, p);
    p->on_rq = 1;
}

static void ttwu_do_activate(struct run_queue *rq, struct task_struct *p)
{
    activate_task(rq, p);
}

static void ttwu_queue(struct task_struct *p, int cpu)
{
    struct run_queue* rq = cpu_rq(cpu);
    ttwu_do_activate(rq, p);
}

#if 0
static int select_task_rq(struct task_struct *p, int cpu, int wake_flags)
{
    /*TODO,暂时默认选择CPU0运行*/
    return 0;
}
#endif
/*
把进程状态设置为TASK_RUNNING，并把该进程插入本地CPU运行队列rq来达到
唤醒睡眠和停止的进程的目的。例如：调用该函数唤醒等待队列中的进程，或
恢复执行等待信号的进程。该函数接受的参数有：
- 被唤醒进程的描述符指针（p）
- 可以被唤醒的进程状态掩码（state）
- 一个标志（sync），用来禁止被唤醒的进程抢占本地CPU上正在运行的进程
如果p进程本身已经是active状态的话。
成功返回 1 成功的话将从等待队列中删除
失败返回 0
*/
static int try_to_wake_up(struct task_struct *p, 
                unsigned int state, int wake_flags)
{
    int cpu;
    preempt_disable();
    if(p == current){
        if(!(READ_ONCE(p->state) & state)){
            goto out;
        }
        WRITE_ONCE(p->state, TASK_RUNNING);
        goto out;
    }
#ifdef CONFIG_SMP

#else
    cpu = task_cpu(p);
#endif
    ttwu_queue(p, cpu);
out:
    preempt_enable();
    return 0;
}

int default_wake_function(wait_queue_entry_t *curr, 
                unsigned mode, int wake_flags, void *key)
{
    return try_to_wake_up(curr->private, mode, wake_flags);
}

static void process_timeout(unsigned long __data)
{
    struct task_struct *p = (struct task_struct *)__data;
    wake_up_process(p);
}

/*
返回值,
0: 表示超时返回
大于0：表示剩余未超时的时间
*/
long schedule_timeout(signed long timeout)
{
    struct timer_list timer;
    unsigned long expire;

    switch(timeout){
        case MAX_SCHEDULE_TIMEOUT:
            schedule();
            goto out;
        default:
        if(timeout < 0){
            printk("schedule_timeout: wrong timeout value: 0x%lx\n", timeout);
            current->state = TASK_RUNNING;
        }
    }

    expire = timeout + jiffies;
    init_timer(&timer);
    timer.expires = expire;
    timer.data = (unsigned long)current;
    timer.function = process_timeout;
    add_timer(&timer);
    schedule();
    del_timer(&timer);

    timeout = expire - jiffies;
out:
    return timeout < 0 ? 0 : timeout;
}
