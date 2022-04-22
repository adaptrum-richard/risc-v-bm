#ifndef __MM_H__
#define __MM_H__
#include "riscv.h"

struct mm_struct {
    pagetable_t pagetable;
};

#endif
