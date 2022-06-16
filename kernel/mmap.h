#ifndef __MMAP_H__
#define __MMAP_H__
#include "mm.h"
#include "types.h"
#include "page.h"

struct vm_area_struct *find_vma(struct mm_struct *mm, unsigned long addr);
int insert_vm_struct(struct mm_struct *mm, struct vm_area_struct *vma);
struct vm_area_struct *vm_area_alloc(struct mm_struct *mm);
int expand_stack(struct vm_area_struct *vma, unsigned long address);
unsigned long brk(unsigned long size);
unsigned long _free(unsigned long addr);
static inline pgoff_t linear_page_index(struct vm_area_struct *vma,
                    unsigned long address)
{
    pgoff_t pgoff;
    pgoff = (address - vma->vm_start) >> PGSHIFT;
    pgoff += vma->vm_pgoff;
    return pgoff;
}

#endif
