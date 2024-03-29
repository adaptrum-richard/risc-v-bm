#ifndef __MMAP_H__
#define __MMAP_H__
#include "mm.h"
#include "types.h"
#include "page.h"

struct vm_area_struct *find_vma(struct mm_struct *mm, unsigned long addr);
int insert_vm_struct(struct mm_struct *mm, struct vm_area_struct *vma);
void vm_area_init(struct vm_area_struct *vma, unsigned long start, 
    unsigned long end, unsigned long flags);
/*删除从start-end的vma,并释放物理内存*/
void remove_vma_region(struct vm_area_struct *head, unsigned long start, unsigned long end);
struct vm_area_struct *vm_area_alloc(struct mm_struct *mm);
struct vm_area_struct *remove_vma(struct vm_area_struct *vma);
int expand_stack(struct vm_area_struct *vma, unsigned long address);
unsigned long brk(unsigned long size);
unsigned long sbrk(unsigned long size);
void show_vma_info(pagetable_t pagatable, struct vm_area_struct *vma);
static inline pgoff_t linear_page_index(struct vm_area_struct *vma,
                    unsigned long address)
{
    pgoff_t pgoff;
    pgoff = (address - vma->vm_start) >> PGSHIFT;
    pgoff += vma->vm_pgoff;
    return pgoff;
}

#endif
