#ifndef __VM_H__
#define __VM_H__
#include "types.h"

void kvminit(void);
void kvminithart();
pagetable_t uvmcreate();
int uvminit(pagetable_t pagetable, uchar *src, uint size);
void set_pgd(pagetable_t pgd);
#endif
