#include "riscv.h"
#include "proc.h"
#include "printk.h"
#include "preempt.h"
#include "sched.h"
#include "bitops.h"
#include "page.h"
#include "string.h"
#include "spinlock.h"
#include "vm.h"
#include "slab.h"

static unsigned long *pid_table;
static struct spinlock task_list_lock;

struct task_struct init_task = INIT_TASK(init_task);

struct task_struct *current = &init_task;

void get_task_list_lock(void)
{
    acquire(&task_list_lock);
}

void free_task_list_lock(void)
{
    release(&task_list_lock);
}

int smp_processor_id()
{
    //int id = r_tp();
    return 0;
}

int get_free_pid(void)
{
    int pid = find_next_zero_bit(pid_table, MAX_PID+1, 0);
    if(pid > MAX_PID)
        pid = -1;
    else
        set_bit(pid, pid_table);
    return pid;
}

void free_pid(int pid)
{
    clear_bit(pid, pid_table);
}

void init_process(void)
{
    struct task_struct *p = &init_task;
    pid_table = (unsigned long*)get_free_page();
    memset(pid_table, 0x0, PGSIZE);

    /*init_task pid 0 used*/
    set_bit(0, pid_table);
    initlock(&task_list_lock, "task list lock");
    p->cpu = smp_processor_id();
    set_task_sched_class(p);
    p->sched_class->enqueue_task(cpu_rq(p->cpu), p);
    w_tp((uint64)current);
}

void free_task(struct task_struct *p)
{
    /*回收子进程资源
    1. stack 
    2. code 
    3. brk
    4. file
    5. pagetable
    6. vma/mm struct
    7. kernel stack
    */
    acquire(&p->lock);
    if(p->state == TASK_ZOMBIE)
        p->state = EXIT_ZOMBIE;
    else 
        goto out;
    release(&p->lock);
    /*打印进程 mmap的页面*/
    //print_all_vma(child->mm->pagetable, child->mm->mmap);

    /*释放stack/code/brk mmap pages and vma*/
    struct vm_area_struct *tmp, *vma = p->mm->mmap;
    for(tmp = vma; tmp;){
        struct vm_area_struct *free_tmp;
        unmap_validpages(p->mm->pagetable, tmp->vm_start, (tmp->vm_end - tmp->vm_start) / PGSIZE);
        free_tmp = tmp;
        tmp = tmp->vm_next;
        kfree(free_tmp);
    }
    /*打印剩余映射page和level 2/1 物理地址*/
    //walk_user_page(child->mm->pagetable, 0);

    /*释放level 2/1 物理地址 和页表基地址*/
    walk_user_page(p->mm->pagetable, 1);
    free_page((unsigned long)p->mm->pagetable);

    /*free mm struct*/
    kfree(p->mm);
        /*从全局task链表中剔除要释放的进程*/
    get_task_list_lock();
    p->prev_task->next_task = p->next_task;
    p->prev_task->next_task->prev_task = p->prev_task;
    free_task_list_lock();

    /*释放pid*/
    free_pid(p->pid);        
    /*释放进程内核stack内存*/
    free_pages((unsigned long)p, get_order(THREAD_SIZE));
out:
    return;
}

struct task_struct *get_zombie_task(void)
{
    struct task_struct *p = NULL;
    get_task_list_lock();
    for(p = init_task.next_task->next_task ; p && p != &init_task; 
                    p = p->next_task){
        acquire(&p->lock);
        if(p->parent == current){
            if(p->state == TASK_ZOMBIE){
                release(&p->lock);
                free_task_list_lock();
                return p;
            }
        }
        release(&p->lock);
    }
    free_task_list_lock();
    return NULL;
}

/*在init_task中调用此函数，检测是否有僵尸进程*/
void free_zombie_task(void)
{
    struct task_struct *p = get_zombie_task();
    if(p)
        free_task(p);
}

struct task_struct *get_child_task(void)
{
    struct task_struct *p = NULL;
    get_task_list_lock();
    for_each_task(p)
    {
        acquire(&p->lock);
        if(p->parent == current){
            release(&p->lock);
            free_task_list_lock();
            return p;
        }
        release(&p->lock);
    }
    free_task_list_lock();
    return NULL;
}

int is_zombie(struct task_struct *p)
{
    return p->state == TASK_ZOMBIE;
}
