#include "exit.h"
#include "proc.h"
#include "wait.h"
#include "printk.h"
#include "log.h"
int do_wait(int *status)
{
    struct task_struct *p = NULL, *child = NULL;
    int free = 0;
    wait_queue_entry_t wq_entry;
    init_wait_entry(&wq_entry, 0);

repeat:
    get_task_list_lock();
    for(p = init_task.next_task; p; p = p->next_task){
        acquire(&p->lock);
        if(p->parent == current){
            if(p->state == TASK_ZOMBIIE)
                free = 1;
            child = p;
            break;
        }
        release(&p->lock);
    }
    free_task_list_lock();
    
    if(child && free == 1){
        //释放子进程资源
        printk("release child resource\n");
        release(&p->lock);
        goto out;
    } else {
        if(!child)
            panic("don't find child thread\n");
        
        prepare_to_wait_event(&child->wait_childexit, &wq_entry, TASK_UNINTERRUPTIBLE);
        release(&p->lock);
        schedule();
        finish_wait(&child->wait_childexit, &wq_entry);
        goto repeat;
    }
    return -1;
out:
    return 0;
}

void do_exit(int code)
{
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
    current->state = TASK_ZOMBIIE;
    release(&current->lock);
    wake_up(&current->wait_childexit);
    
    schedule();
    panic("zombie exit");
}