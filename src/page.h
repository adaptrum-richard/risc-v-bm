#ifndef __PAGE_H__
#define __PAGE_H__
#include "memlayout.h"
#include "mm.h"

extern struct page *mem_map;

#define ARCH_PFN_OFFSET (ARCH_PHYS_OFFSET >> PGSHIFT)

#define PGSIZE 4096 // bytes per page
#define PGSHIFT 12  // bits of offset within a page

#define PGROUNDUP(sz) (((sz) + PGSIZE - 1) & ~(PGSIZE - 1))
#define PGROUNDDOWN(a) (((a)) & ~(PGSIZE - 1))

#define PAGE_MASK (~(PGSIZE-1))
#define PAGE_ALIGN(addr) (((addr) + PGSIZE - 1) & PAGE_MASK)

#define page_to_pfn(page) ((unsigned long)(page - mem_map) + ARCH_PFN_OFFSET)

#define PFN_UP(x) (((x) + PGSIZE - 1) >> PGSHIFT)
#define PFN_DOWN(x)  (x >> PGSHIFT)

/*TODO:实现__va __pa*/
#define __va(x) (x)
#define __pa(x) (x)

#define pfn_to_page(pfn) (mem_map + ((pfn) - ARCH_PFN_OFFSET))
#define page_to_address(page) __va(page_to_pfn(page) << PGSHIFT)
#define virt_to_page(addr)  (pfn_to_page(__pa(addr) >> PGSHIFT))

static inline int get_order(unsigned long size)
{
    int order;
    size--;
    size >>= PGSHIFT;
    order =fls64(size);
    return order;
}

//func
void memblock_free_pages(struct page *page, unsigned int order);
void clear_page_order(struct page *page);
void set_page_order(struct page *page, unsigned int order);
void free_pages(unsigned long addr, unsigned int order);
void free_page(unsigned long addr);
void free_area_init_node(int nid, unsigned long node_start_pfn,
        unsigned long *zone_size);
unsigned long get_free_page(void);
unsigned long get_free_pages(unsigned int order);
void show_buddyinfo(void);
#endif
