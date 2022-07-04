#ifndef __PROC_H__
#define __PROC_H__
#include "types.h"
#include "spinlock.h"
#include "file.h"
#include "param.h"
#include "list.h"
#include "sched.h"
#include "wait.h"

#define THREAD_SIZE     ((1<<12)*2) //8K
#define PF_KTHREAD      (0x10000)
#define MAX_PID         (4096*8 -1)

#define TASK_RUNNING			0x0000
#define TASK_INTERRUPTIBLE		0x0001
#define TASK_UNINTERRUPTIBLE	0x0002
#define TASK_ZOMBIE            0x0004
#define TASK_STOPPED            0X0008
#define TASK_NORMAL			(TASK_INTERRUPTIBLE | TASK_UNINTERRUPTIBLE)


#define EXIT_DEAD			0x0010
#define EXIT_ZOMBIE			0x0020


struct thread_struct {
    /* Callee-saved registers */
    uint64 ra;
    uint64 sp;/* Kernel mode stack */
    //callee-saved
    uint64 s[12]; /*s0 - s11, s[0]: frame pointer */
};

struct task_struct {
    struct thread_struct thread;
    uint64 kernel_sp;//112 offset
    uint64 user_sp;//在进入异常时保存用户的sp  120 offset
    uint64 preempt_count; //128 offset
    uint64 need_resched; //136 offset
    struct mm_struct *mm;
    int cpu;
    int counter;
    int pid;
    uint64 state;
    int flags;
    struct spinlock lock;
    char name[16]; //task name
    uint64 priority;
    int on_rq;
    struct sched_class *sched_class;
    struct file *ofile[NOFILE]; //open file
    struct inode *cwd; //当前的目录
    struct list_head run_list;
    struct task_struct *parent;
    struct task_struct *prev_task, *next_task;
    wait_queue_head_t wait_childexit;
};

static inline unsigned int task_cpu(const struct task_struct *p)
{
    return READ_ONCE(p->cpu);
}

#define INIT_TASK(task) \
{  \
    .thread = {0},  \
    .counter = 1, \
    .lock = INIT_SPINLOCK(task), \
    .name = ""#task"",    \
    .user_sp = 0x55555, \
    .kernel_sp = 0x6666, \
    .pid = 0,   \
    .preempt_count = 0, \
    .state = TASK_RUNNING,   \
    .flags = PF_KTHREAD | TASK_NORMAL,    \
    .priority = 5,  \
    .need_resched = 0,  \
    .on_rq = 0, \
    .run_list = LIST_HEAD_INIT(task.run_list), \
    .parent = NULL, \
    .cwd = NULL, \
    .next_task = (struct task_struct *)&task, \
    .prev_task = (struct task_struct *)&task, \
    .wait_childexit = {  \
        .lock = INIT_SPINLOCK("init_task"),  \
        .head = LIST_HEAD_INIT(task.wait_childexit.head) \
    }, \
}

extern struct task_struct *current;
extern struct task_struct *task[];
extern struct task_struct init_task;
extern int nr_tasks;
int smp_processor_id();
void init_process(void);

int get_free_pid(void);
void free_pid(int pid);

void get_task_list_lock(void);
void free_task_list_lock(void);
#define LOCK_TASK_LIST() (get_task_list_lock())
#define UNLOCK_TASK_LIST() (free_task_list_lock())
void free_task(struct task_struct *p);
struct task_struct *get_zombie_task(void);
void free_zombie_task(void);
struct task_struct *get_child_task(void);
int is_zombie(struct task_struct *p);
#endif
