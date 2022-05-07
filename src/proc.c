#include "riscv.h"
#include "proc.h"
#include "printk.h"
#include "preempt.h"

static struct task_struct init_task = {
    .thread = {0},
    .counter = 15,
    .lock = {
        .locked = 0,
        .name = "init_task"},
    .name = "init_task",
    .pid = 0,
    .preempt_count = 0,
    .state = RUNNING,
    .flags = PF_KTHREAD,
    .priority = 5,
    .chan = 0,
    .wait = 0,
    .need_resched = 1,
    .runtime = 0
};

struct task_struct *current = &init_task;
struct task_struct *task[NR_TASKS] = {
    &init_task,
};
int nr_tasks = 1;

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

int cpuid()
{
    int id = r_tp();
    return id;
}
