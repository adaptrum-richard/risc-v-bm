#include "sched.h"
#include "printk.h"
#include "proc.h"
#include "list.h"

static void dequeue_task_simple(struct run_queue *rq,
    struct task_struct *p)
{
    if(p->on_rq == 1){
        rq->nr_running--;
        p->on_rq = 0;
        list_del(&p->run_list);
    }
}

static void enqueue_task_simple(struct run_queue *rq,
    struct task_struct *p)
{
    if(p->on_rq == 0){
        list_add(&p->run_list, &rq->rq_head);
        p->on_rq = 1;
        rq->nr_running++;
    }
}

static int goodness(struct task_struct *p)
{
    int weight;
    weight = p->counter;
    return weight;
}

static void reset_score(void)
{
    struct task_struct *p;
    for_each_task(p){
        if(p->counter <= 0){
            p->counter = DEF_COUNTER + p->priority;
        }
    }
    if(init_task.counter <=  0)
        init_task.counter = DEF_COUNTER + init_task.priority;
}

static struct task_struct *pick_next_task_simple(struct run_queue *rq,
    struct task_struct *prev)
{
    struct task_struct *p, *next;
    struct list_head *tmp;
    int weight;
    int c;
repeat:
    c = -1000;
    list_for_each(tmp, &rq->rq_head) {
        p = list_entry(tmp, struct task_struct, run_list);
        weight = goodness(p);
        if(weight > c){
            c = weight;
            next = p;
        }
    }
    if(!c){ 
        reset_score();
        goto repeat;
    }
    return next;
}

static void task_tick_simple(struct run_queue *rq, struct task_struct *p)
{
    p->counter--;
    if(p->counter <= 0){
        p->counter = 0;
        /*当时间片用完时，设置当前进程可以在退出中断前被抢占*/
        p->need_resched = 1;
    }
}

const struct sched_class simple_sched_class = {
    .next = NULL,
    .dequeue_task = dequeue_task_simple,
    .enqueue_task =  enqueue_task_simple,
    .task_tick = task_tick_simple,
    .pick_next_task = pick_next_task_simple,
};
