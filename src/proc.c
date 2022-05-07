#include "riscv.h"
#include "proc.h"

static struct task_struct init_task = {
    .thread = {0},
    .counter = 0,
    .lock = {
        .locked = 0,
        .name = "init_task"},
    .name = "init_task",
    .pid = 0,
    .preempt_count = 0,
    .state = UNUSED,
    .flags = PF_KTHREAD,
    .priority = 5,
    .chan = 0,
    .wait = 0,
};

struct task_struct *current = &init_task;
struct task_struct *task[NR_TASKS] = {
    &init_task,
};
int nr_tasks = 1;

int cpuid()
{
    int id = r_tp();
    return id;
}
