#ifndef __KALLOC_H__
#define __KALLOC_H__
#include "spinlock.h"

struct run {
  struct run *next;
};

struct kmem{
  struct spinlock lock;
  struct run *freelist;
};

void kinit(void);
void* kalloc(void);
void kfree(void *pa);
void *get_free_one_page(void);
#endif
