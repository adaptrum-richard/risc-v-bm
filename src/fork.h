#ifndef __FORK_H__
#define __FORK_H__

#include "types.h"

int copy_process(uint64 clone_flags, uint64 fn, uint64 arg, char *name);

#endif
