#ifndef __PROC_H__
#define __PROC_H__
#include "types.h"
#include "spinlock.h"
#include "file.h"
#include "param.h"

#define THREAD_SIZE     (1<<12)
#define PF_KTHREAD      (1UL<<63)
#define NR_TASKS        0xf
enum procstate { 
    UNUSED, 
    USED, 
    SLEEPING,
    RUNNABLE,
    RUNNING,
    ZOMBIE,
};

struct thread_struct {
    /* Callee-saved registers */
    uint64 ra;
    uint64 sp;/* Kernel mode stack */
    //callee-saved
    uint64 s[12]; /*s0 - s11, s[0]: frame pointer */
};

struct task_struct {
    struct thread_struct thread;
    int counter;
    uint64 state;
    uint64 preempt_count;
    int pid;
    struct spinlock lock;
    char name[16]; //task name
    uint64 flags;
    uint64 priority;
    uint64 chan;
    uint64 wait;
    uint64 need_resched;
    uint64 runtime;
    struct file *ofile[NOFILE]; //open file
};

extern struct task_struct *current;
extern struct task_struct *task[];
extern int nr_tasks;
int cpuid();
void print_task_info();
#endif
