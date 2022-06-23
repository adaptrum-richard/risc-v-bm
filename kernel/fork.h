#ifndef __FORK_H__
#define __FORK_H__

#include "types.h"

int copy_process(uint64 clone_flags, uint64 fn, uint64 arg, char *name);
int move_to_user_mode(unsigned long start, unsigned long size, 
        unsigned long pc);
int do_fork(void);
#endif
