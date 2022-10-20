#ifndef __SLAB_H__
#define __SLAB_H__
#include "types.h"
#include "list.h"
  
extern struct kmem_cache *boot_kmem_cache;

struct kmem_cache {
    unsigned int object_size;
    unsigned int size;
    unsigned int align;
    unsigned long flags;
    const char *name;
    int refcount;
    struct list_head list;
};

void *kmem_cache_zalloc(struct kmem_cache *s);
void slob_free(void *block, int size);
void *slob_alloc(size_t size, int align);
int __kmem_cache_create(struct kmem_cache *s,
		unsigned long flags);
void kmem_cache_init(void);
void kmem_cache_free(struct kmem_cache *s, void *b);
void *kmalloc(size_t size);
void kfree(const void *block);
struct kmem_cache *kmem_cache_create(const char *name, 
    size_t size, size_t align, unsigned long flags);
void test_slob(void);
#endif
