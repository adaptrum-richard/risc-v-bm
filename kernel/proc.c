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
#include "list.h"

static unsigned long *pid_table;
static struct spinlock pid_lock; 
static struct spinlock task_list_lock[NCPU];

struct task_struct init_task = INIT_TASK(init_task);
struct task_struct *init_task_array[NCPU] = {NULL,};
__attribute__((aligned(1024))) struct task_struct init_task_mem[NCPU][1024] = {0};
struct task_struct *get_init_task()
{
    return init_task_array[smp_processor_id()];
}

void set_init_task_to_current()
{
    w_current((uint64)&init_task);
}

int smp_processor_id()
{
    return current->cpu;
}

void get_task_list_lock(void)
{
    acquire(&task_list_lock[smp_processor_id()]);
}

void free_task_list_lock(void)
{
    release(&task_list_lock[smp_processor_id()]);
}
extern volatile int init_done_flag;
int get_free_pid(void)
{
    acquire(&pid_lock);
    int pid = find_next_zero_bit(pid_table, MAX_PID+1, 0);
    if(pid > MAX_PID)
        pid = -1;
    else
        set_bit(pid, pid_table);
    release(&pid_lock);
    return pid;
}

void free_pid(int pid)
{
    acquire(&pid_lock);
    clear_bit(pid, pid_table);
    release(&pid_lock);
}
void init_task_struct(struct task_struct *p)
{
    sprintf(p->name, "init_task%d", smp_processor_id());
    memset(&p->thread, 0, sizeof(p->thread));
    p->counter = DEF_COUNTER;
    initlock(&p->lock, p->name);
    p->user_sp = 0x55555;
    p->kernel_sp = 0x6666;
    //p->pid = smp_processor_id();
    p->preempt_count = 0;
    p->state = TASK_RUNNING;
    p->flags = PF_KTHREAD | TASK_NORMAL;
    p->priority = 5;
    p->need_resched = 0;
    p->on_rq = 0;
    p->run_list.next = &p->run_list;
    p->run_list.prev = &p->run_list;
    p->parent = NULL;
    p->cwd = NULL;
    p->next_task = p;
    p->prev_task = p;
    initlock(&p->wait_childexit.lock, p->name);
    p->wait_childexit.head.next = p->wait_childexit.head.prev =
        &p->wait_childexit.head;
}


void init_process(int cpu)
{
    struct task_struct *p;
    if(cpu == 0){
        p = &init_task;
        for(int i = 0; i < NCPU; i++){
            initlock(&task_list_lock[i], "task list lock");
        }
        initlock(&pid_lock, "pid_lock");
        pid_table = (unsigned long*)get_free_page();
        memset(pid_table, 0x0, PGSIZE);
    }else{
        p = (struct task_struct *)init_task_mem[cpu];
        p->cpu = cpu;
        /*先设置task_struct到tp寄存器,sched_init函数会使用current*/
        w_current((uint64)p);
        init_task_struct(p);
        sched_init();
    }

    p->pid = get_free_pid();
    init_task_array[smp_processor_id()] = p;
    set_task_sched_class(p);
    p->sched_class->enqueue_task(cpu_rq(p->cpu), p);
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
    struct task_struct *init_p = get_init_task();
    get_task_list_lock();
    for(p = init_p->next_task->next_task ; p && p != init_p; 
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
    for_each_task(p, get_init_task())
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
