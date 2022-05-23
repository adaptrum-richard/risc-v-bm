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

#endif
