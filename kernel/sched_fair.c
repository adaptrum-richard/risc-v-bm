#include "sched_fair.h"
#include "types.h"
#include "sched.h"
#include "proc.h"

static unsigned int sched_nr_latency = 8;

static inline struct task_struct *task_of(struct sched_entity *se)
{
	return container_of(se, struct task_struct, se);
}

static inline void update_load_add(struct load_weight *lw, unsigned long inc)
{
	lw->weight += inc;
	lw->inv_weight = 0;
}

static inline struct cfs_rq *cfs_rq_of(struct sched_entity *se)
{
	struct task_struct *p = task_of(se);
	struct run_queue *rq = task_rq(p);

	return &rq->cfs;
}

static inline uint64 mul_u64_u32_shr(uint64 a, uint32 mul, unsigned int shift)
{
	return (uint64)(((unsigned __int128)a * mul) >> shift);
}

static uint64 __sched_period(unsigned long nr_running)
{
    if(nr_running > sched_nr_latency)
        return nr_running * sched_min_granularity;
    else
        return sched_latency;
}

#define WMULT_CONST    (~0U)
#define WMULT_SHIFT    32

static void __update_inv_weight(struct load_weight *lw)
{
    unsigned long w;

    if (lw->inv_weight)
        return;

    w = scale_load_down(lw->weight);

    if (w >= WMULT_CONST)
        lw->inv_weight = 1;
    else if (!w)
        lw->inv_weight = WMULT_CONST;
    else
        lw->inv_weight = WMULT_CONST / w;
}

/*
* 函数的作用
  1. 计算虚拟时间
  2. 计算从调度器瓜分的时间
* __calc_delta()函数参数解析

  1.当计算虚拟时间时
    delta_exec：表示这段时间内进程实际运行在CPU上的时间
    weight：表示NICE值为0的进程的权重(根据优先级经过[数组](https://elixir.bootlin.com/linux/v5.0/source/kernel/sched/core.c#L7044)转换得到)。
    lw：表示当前虚拟计算的进程的权重

  2.计算从调度器瓜分的时间
    delta_exec:表示cfs调度器的调度周期
    weight:表示进程的权重(根据优先级经过[数组](https://elixir.bootlin.com/linux/v5.0/source/kernel/sched/core.c#L7044)转换得到)。
    lw:表示cfs调度器中可运行进程的权重和。

    delta_exec*weight/lw.weight  或  (delta_exec * (weight * lw->inv_weight)) >> WMULT_SHIFT
*/
static uint64 __calc_delta(uint64 delta_exec, unsigned long weight, struct load_weight *lw)
{
    uint64 fact = scale_load_down(weight);
    int shift = WMULT_SHIFT;

    __update_inv_weight(lw);

    if (fact >> 32) {
        while (fact >> 32) {
            fact >>= 1;
            shift--;
        }
    }

    /* hint to use a 32x32->64 mul */
    fact = (uint64)(uint32)fact * lw->inv_weight;

    while (fact >> 32) {
        fact >>= 1;
        shift--;
    }

    return mul_u64_u32_shr(delta_exec, fact, shift);
}

static inline uint64 calc_delta_fair(uint64 delta, struct sched_entity *se)
{
	if (se->load.weight != NICE_0_LOAD)
		delta = __calc_delta(delta, NICE_0_LOAD, &se->load);

	return delta;
}
/*
 *
*/
static uint64 sched_slice(struct cfs_rq *cfs_rq, struct sched_entity *se)
{
	uint64 slice = __sched_period(cfs_rq->nr_running + !se->on_rq);

    struct load_weight *load;
    struct load_weight lw;

    cfs_rq = cfs_rq_of(se);
    load = &cfs_rq->load;

    if (!se->on_rq) {
        lw = cfs_rq->load;

        update_load_add(&lw, se->load.weight);
        load = &lw;
    }
    slice = __calc_delta(slice, se->load.weight, load);
	
	return slice;
}

static inline struct cfs_rq *task_cfs_rq(struct task_struct *p)
{
	return &task_rq(p)->cfs;
}

void task_fork_fair(struct task_struct *p)
{
    struct cfs_rq *cfs_rq;
    struct sched_entity *se = &p->se, *curr;
    struct run_queue *rq = this_rq();
    acquire(&rq->lock);
    update_rq_clock(rq);
    cfs_rq = task_cfs_rq(current);
    release(&rq->lock);
}

void enqueue_task_fair(struct run_queue *rq, struct task_struct *p)
{

}

void dequeue_task_fair(struct run_queue *rq, struct task_struct *p)
{

}

void task_tick_fair(struct run_queue *rq, struct task_struct *p)
{

}

struct task_struct *pick_next_task_fair(struct run_queue *rq,
    struct task_struct *prev)
{

}

const struct sched_class fair_sched_class = {
    .next = NULL,
    .task_fork = task_fork_fair,
    .dequeue_task = dequeue_task_fair,
    .enqueue_task =  enqueue_task_fair,
    .task_tick = task_tick_fair,
    .pick_next_task = pick_next_task_fair,
};
