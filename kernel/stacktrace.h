#ifndef __STACKTRACE_H__
#define __STACKTRACE_H__

#include "pt_regs.h"
#include "proc.h"

struct stackframe {
	unsigned long fp;
	unsigned long sp;
	unsigned long pc;
};

void dump_backtrace(struct pt_regs *regs, struct task_struct *p);
void show_regs(struct pt_regs *regs);

#endif
