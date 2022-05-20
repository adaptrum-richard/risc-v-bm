#include "riscv.h"
#include "proc.h"
#include "printk.h"
#include "preempt.h"
#include "sched.h"
#include "biops.h"
#include "kalloc.h"
#include "string.h"
#include "spinlock.h"

static unsigned long *pid_table;
static struct spinlock task_list_lock;

struct task_struct init_task = INIT_TASK(init_task);

struct task_struct *current = &init_task;
struct task_struct *task[NR_TASKS] = {
    &init_task,
};
int nr_tasks = 1;

void get_task_list_lock(void)
{
    acquire(&task_list_lock);
}

void free_task_list_lock(void)
{
    release(&task_list_lock);
}

void print_task_info()
{
    char buf[1024] = {0};
    char *p = buf;
    for(int i = 0; i < nr_tasks; i++){
        p += sprintf(p, "task name:%16s, state:%d, preempt_count: 0x%lx, counter: %d\n", 
            task[i]->name, task[i]->state, task[i]->preempt_count, task[i]->counter);
    }
    p += sprintf(p, "===================\n");
    if(in_hardirq())
        printk_intr("interrupt print: \n%s\n", buf);
    else
        printk("%s", buf);
}

int smp_processor_id()
{
    int id = r_tp();
    return id;
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
    pid_table = (unsigned long*)get_free_one_page();
    memset(pid_table, 0x0, PGSIZE);

    /*init_task pid 0 used*/
    set_bit(0, pid_table);
    initlock(&task_list_lock, "task list lock");
    p->cpu = smp_processor_id();
    set_task_sched_class(p);
    p->sched_class->enqueue_task(cpu_rq(p->cpu), p);
}
