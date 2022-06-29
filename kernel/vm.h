#ifndef __VM_H__
#define __VM_H__
#include "types.h"
#include "mm.h"

void kvminit(void);
void kvminithart();
pagetable_t uvmcreate();
int vm_map_program(pagetable_t pagetable, uint64 offset, uchar *src, uint size);
pagetable_t get_kpgtbl();
pagetable_t copy_kernel_tbl(void);
int mappages(pagetable_t pagetable, uint64 va, uint64 size, uint64 pa, int perm);
void uvmunmap(pagetable_t pagetable, uint64 va, uint64 npages, int do_free);
void unmap_validpages(pagetable_t pagetable, uint64 va, uint64 npages);
int space_data_copy_out(uint64 dst_va, void *src_data_addr, uint len);
int spcae_data_copy_in(void *dst_data_addr, uint64 src_va, uint64 len);
uint64 walkaddr(pagetable_t pagetable, uint64 va);
pte_t *walk(pagetable_t pagetable, uint64 va, int alloc);
int vm_map_normal_mem(pagetable_t pagetable, uint64 vm_base, uchar *src, uint size);
void free_pagtable_and_vma(pagetable_t pagetable, struct vm_area_struct *head);
void freewalk(pagetable_t pagetable);
#endif
