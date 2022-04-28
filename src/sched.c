#include "sched.h"
#include "proc.h"
#include "riscv.h"
#include "printk.h"

volatile uint64 jiffies = 0;

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
    //asm volatile("fence rw, rw");
    cpu_switch_to(&prev->thread, &next->thread);
    //asm volatile("fence rw, rw");
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

    if(current == task[next] && current != task[1]) {
        task[1]->counter = 15;
        switch_to(task[1]);
    }else
        switch_to(task[next]);
    preempt_enable();
}

void schedule(void)
{
    current->counter = 0;
    __schedule();
}

void wakeup(uint64 chan)
{
    struct task_struct *p;
    for(int i = 0; i < NR_TASKS; i++){
        p = task[i];
        if(p){
            acquire(&p->lock);
            if(p->state == SLEEPING && p->chan == chan ){
                p->state = RUNNING;
            }
            release(&p->lock);
        }
    }
}

void sleep(uint64 sec)
{
    preempt_disable();
    acquire(&current->lock);
    current->chan = sec*HZ +jiffies;
    current->state = SLEEPING;
    release(&current->lock);
    
    __schedule();

    acquire(&current->lock);
    if(current->chan == 0)
        panic("sleep");
    current->chan = 0;
    release(&current->lock);
    preempt_disable();
}

void wake(uint64 wait)
{
    struct task_struct *p;
    for(int i = 0; i < NR_TASKS; i++){
        p = task[i];
        if(p){
            acquire(&p->lock);
            if(p->state == SLEEPING && p->wait == wait ){
                p->state = RUNNING;
            }
            release(&p->lock);
        }
    }
}

void wait(uint64 c)
{
    preempt_disable();
    acquire(&current->lock);
    current->wait = c;
    current->state = SLEEPING;
    release(&current->lock);

    __schedule();
   
    acquire(&current->lock);
    if(current->wait == 0)
        panic("wait");
    current->wait = 0;
    release(&current->lock);
    preempt_enable();
}

void timer_tick(void)
{
    current->counter--;
    if(cpuid() == 0){
        jiffies++;
        wakeup(jiffies);
    }
    if(current->counter > 0 || current->preempt_count > 0)
        return;
    schedule();
}
