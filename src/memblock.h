#ifndef __MEMBLOCK_H__
#define __MEMBLOCK_H__

#define MEMBLOCK_FREE 0
#define MEMBLOCK_RESERVE 1

struct memblock_region {
    unsigned long base;
    unsigned long size;
    unsigned int flags;
    unsigned int allocated;
    struct memblock_region *prev, *next;
};

struct memblock {
    struct memblock_region head;
    unsigned int num_regions;
    unsigned long total_size;
};

#define MAX_MEMBLOCK_REGIONS 64
#define for_each_memblock_region(mrg) \
    for (mrg = memblock.head.next; mrg; mrg = mrg->next)

void *memblock_virt_alloc(unsigned long size);
int memblock_add_region(unsigned long base, unsigned long size);
int memblock_reserve(unsigned long base, unsigned long size);
void memblock_dump_region(void);
unsigned long memblock_free_all(void);
#endif
