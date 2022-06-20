#ifndef __VM_H__
#define __VM_H__
#include "types.h"

void kvminit(void);
void kvminithart();
pagetable_t uvmcreate();
int uvminit(pagetable_t pagetable, uchar *src, uint size);
pagetable_t get_kpgtbl();
pagetable_t copy_kernel_tbl(void);
int mappages(pagetable_t pagetable, uint64 va, uint64 size, uint64 pa, int perm);
void uvmunmap(pagetable_t pagetable, uint64 va, uint64 npages, int do_free);
void uvunmap_validpages(pagetable_t pagetable, uint64 va, uint64 npages);
int space_data_copy_out(uint64 dst_va, void *src_data_addr, uint len);
int spcae_data_copy_in(void *dst_data_addr, uint64 src_va, uint64 len);
uint64 walkaddr(pagetable_t pagetable, uint64 va);
#endif
