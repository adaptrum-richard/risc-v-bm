#include "sched.h"
#include "proc.h"
#include "riscv.h"
#include "printk.h"
#include "preempt.h"

volatile uint64 jiffies = 0;

extern void cpu_switch_to(struct thread_struct *prev, struct thread_struct *next);

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

            
            if(p && p->state == RUNNABLE){
                if(in_hardirq())
                    acquire(&p->lock);
                else
                    acquire_irq(&p->lock);
                if( p->state == RUNNABLE)
                    p->state = RUNNING;
                if(in_hardirq())
                    release(&p->lock);
                else
                    release_irq(&p->lock);
            }

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

void wakeup(uint64 chan)
{
    struct task_struct *p;
    for(int i = 0; i < NR_TASKS; i++){
        p = task[i];
        if(p){
            acquire(&p->lock);
            if(p->state == SLEEPING && p->chan == chan ){
                p->state = RUNNABLE;
            }
            release(&p->lock);
        }
    }
}

void sleep(uint64 sec)
{
    preempt_disable();
    acquire_irq(&current->lock);
    current->chan = sec*HZ +jiffies;
    current->state = SLEEPING;
    release_irq(&current->lock);
    
    __schedule();

    acquire_irq(&current->lock);
    if(current->chan == 0)
        panic("sleep");
    current->chan = 0;
    release_irq(&current->lock);
    preempt_enable();
}

void wake(uint64 wait)
{
    struct task_struct *p;
    for(int i = 0; i < NR_TASKS; i++){
        p = task[i];
        if(p){
            if(in_hardirq())
                acquire(&p->lock);
            else
                acquire_irq(&p->lock);
            if(p->wait == wait && p->state == SLEEPING){
                p->state = RUNNABLE;
            }
            if(in_hardirq())
                release(&p->lock);
            else
                release_irq(&p->lock);
        }
    }
}

void wait(uint64 c)
{
    preempt_disable();
    acquire_irq(&current->lock);
    current->wait = c;
    //说明已经中断已经发生了，防止等待进程无法被唤醒
    if(current->state == RUNNABLE) 
        current->state = SLEEPING;
    release_irq(&current->lock);

    __schedule();
   
    /*如果中断没有发送，大不了再来一次。*/
    acquire_irq(&current->lock);
    if(current->wait == 0)
        panic("wait");
    current->wait = 0;
    release_irq(&current->lock);
    preempt_enable();
}

void timer_tick(void)
{
    current->counter--;
    if(cpuid() == 0){
        jiffies++;
        wakeup(jiffies);
    }
    //print_task_info();
    if(current->counter > 0)
        return;
    current->need_resched = 1;
}
