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
int copy_kernel_to_user(pagetable_t pagetable, uint64 dstva, char *src, uint64 len);
int copy_user_to_kernel(pagetable_t pagetable, char *dst, uint64 srcva, uint64 len);
#endif
