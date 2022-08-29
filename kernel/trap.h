#ifndef __TRAP_H__
#define __TRAP_H__
#include "proc.h"
#include "pt_regs.h"

void trapinithart(void);
void dump_backtrace(struct pt_regs *regs, struct task_struct *p);
#endif
