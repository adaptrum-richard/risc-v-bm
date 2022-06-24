#ifndef __EXEC_H__
#define __EXEC_H__
#include "types.h"

int read_initcode(uint64 *prog_start, uint64 *prog_size, uint64 *pc);
#endif
