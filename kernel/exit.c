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
    struct task_struct *p = NULL, *child = NULL;
    int free = 0;
    int result = -1;
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
        result = child->pid;
        /*回收子进程资源
        1. stack 
        2. code 
        3. brk
        4. file
        5. pagetable
        6. vma/mm struct
        7. kernel stack
        */

        /*打印进程 mmap的页面*/
        //print_all_vma(child->mm->pagetable, child->mm->mmap);

        /*释放stack/code/brk mmap pages and vma*/
        struct vm_area_struct *tmp, *vma = child->mm->mmap;
        for(tmp = vma; tmp;){
            struct vm_area_struct *free_tmp;
            unmap_validpages(child->mm->pagetable, tmp->vm_start, (tmp->vm_end - tmp->vm_start) / PGSIZE);
            free_tmp = tmp;
            tmp = tmp->vm_next;
            kfree(free_tmp);
        }
        /*打印剩余映射page和level 2/1 物理地址*/
        //walk_user_page(child->mm->pagetable, 0);

        /*释放level 2/1 物理地址 和页表基地址*/
        walk_user_page(child->mm->pagetable, 1);
        free_page((unsigned long)child->mm->pagetable);

        /*free mm struct*/
        kfree(child->mm);

         /*从全局task链表中剔除要释放的进程*/
        get_task_list_lock();
        child->prev_task->next_task = child->next_task;
        free_task_list_lock();

        /*释放pid*/
        free_pid(child->pid);
        release(&p->lock);
        
        /*释放进程内核stack内存*/
        free_pages((unsigned long)child, get_order(THREAD_SIZE));
        
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
    current->state = TASK_ZOMBIIE;
    release(&current->lock);

    spin_lock_irqsave(&(cpu_rq(smp_processor_id())->lock),flags);
    current->sched_class->dequeue_task(cpu_rq(task_cpu(current)), current);
    current->on_rq = 0;
    spin_unlock_irqrestore(&(cpu_rq(smp_processor_id())->lock), flags);

    wake_up(&current->wait_childexit);

    schedule();
    panic("zombie exit");
}