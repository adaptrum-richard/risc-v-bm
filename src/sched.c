#include "sched.h"
#include "proc.h"
#include "riscv.h"
#include "printk.h"

extern void cpu_switch_to(struct thread_struct *prev, struct thread_struct *next);

void preempt_disable(void)
{
    current->preempt_count++;
}

void preempt_enable(void)
{
    current->preempt_count--;
}

void schedule_tail(void)
{
    preempt_enable();
    intr_on();
}

void switch_to(struct task_struct *next)
{
    if(current == next){
        return;
    }
    struct task_struct *prev = current;
    current = next;
    asm volatile("fence rw, rw");
    cpu_switch_to(&prev->thread, &next->thread);
    asm volatile("fence rw, rw");
}

void __schedule(void)
{
    preempt_disable();
    int next, c;
    struct task_struct *p;
    while(1){
        c = -1;
        next = 0;
        for(int i = 0; i < NR_TASKS; i++){
            p = task[i];
            if(p && p->state == RUNNING && p->counter > c){
                c = p->counter;
                next = i;
            }
        }
        if(c){
            break;
        }
        for(int i = 0; i < NR_TASKS; i++){
            p = task[i];
            if(p)
                p->counter = (p->counter >> 1) + p->priority;
        }
    }
    switch_to(task[next]);
    preempt_enable();
}

void schedule(void)
{
    current->counter = 0;
    __schedule();
}

void timer_tick(void)
{
    current->counter--;
    if(current->counter > 0 || current->preempt_count > 0)
        return;
    schedule();
}
