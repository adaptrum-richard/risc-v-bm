#include "exit.h"
#include "proc.h"
#include "wait.h"
#include "printk.h"
#include "log.h"
#include "spinlock.h"
#include "mm.h"
#include "vm.h"
#include "memlayout.h"
#include "slab.h"
#include "page.h"
#include "mmap.h"

int do_wait(int *status)
{
    struct task_struct *child = NULL;
    int result = -1;
    wait_queue_entry_t wq_entry;
    init_wait_entry(&wq_entry, 0);
    /*获取当前进程下的一个僵尸进程*/
do_try:
    child = get_child_task();
    if(child && is_zombie(child)){
        result = child->pid;
        free_task(child);
        goto out;
    } else {
        if(!child)
            panic("don't find child thread\n");
        prepare_to_wait_event(&child->wait_childexit, &wq_entry, TASK_UNINTERRUPTIBLE);
        schedule();
        finish_wait(&child->wait_childexit, &wq_entry);
        goto do_try;
    }

out:
    return result;
}

void do_exit(int code)
{
    unsigned long flags;
    if(current == &init_task){
        panic("init_task can't exit\n");
    }

    for(int fd = 0; fd < NOFILE; fd++){
        if(current->ofile[fd]){
            struct file *f = current->ofile[fd];
            fileclose(f);
            current->ofile[fd] = 0;
        }
    }

    log_begin_op();
    if(current->cwd)
        iput(current->cwd);
    log_end_op();

    acquire(&current->lock);
    current->state = TASK_ZOMBIE;
    release(&current->lock);

    spin_lock_irqsave(&(cpu_rq(smp_processor_id())->lock),flags);
    current->sched_class->dequeue_task(cpu_rq(task_cpu(current)), current);
    current->on_rq = 0;
    spin_unlock_irqrestore(&(cpu_rq(smp_processor_id())->lock), flags);

    if(current->parent == NULL){
        current->parent = &init_task;
    }else 
        wake_up(&current->wait_childexit);

    schedule();
    panic("zombie exit");
}