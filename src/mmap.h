#ifndef __MMAP_H__
#define __MMAP_H__
#include "mm.h"
#include "types.h"
struct vm_area_struct *find_vma(struct mm_struct *mm, unsigned long addr);
int insert_vm_struct(struct mm_struct *mm, struct vm_area_struct *vma);
struct vm_area_struct *vm_area_alloc(struct mm_struct *mm);
#endif
