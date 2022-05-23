#ifndef __MM_H__
#define __MM_H__
#include "riscv.h"

struct vm_area_struct;
struct mm_struct {
    struct vm_area_struct *mmap;
    pagetable_t pagetable;
};

struct vm_area_struct {
    unsigned long vm_start;
    unsigned long vm_end;
    struct mm_struct *vm_mm;
    struct vm_area_struct *vm_next, *vm_prev;
    unsigned long vm_pgoff;
    unsigned long vm_flags;
};

#endif
