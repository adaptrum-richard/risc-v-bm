#ifndef __FORK_H__
#define __FORK_H__

#include "types.h"
#include "proc.h"

int copy_process(uint64 clone_flags, uint64 fn, uint64 arg, char *name);
int move_to_user_mode(unsigned long start, unsigned long size, 
        unsigned long pc);
int do_fork(void);
void set_user_mode_epc(struct task_struct *tsk, uint64 epc);
void set_user_mode_sp(struct task_struct *tsk, uint64 sp);
void print_epc(struct task_struct *tsk);
void print_regs_sp(struct task_struct *tsk);
void print_regs_tp(struct task_struct *tsk);
void set_user_mode_a1(struct task_struct *tsk, uint64 a1);
int create_kernel_thread(void (*fn)(unsigned long arg), unsigned long arg, char *name);
#endif
