#ifndef __MM_H__
#define __MM_H__
#include "riscv.h"
#include "list.h"
#include "atomic.h"
#include "biops.h"

#define MAX_ORDER 11
#define MAX_ORDER_NR_PAGES (1 << (MAX_ORDER - 1))

struct vm_area_struct;
struct mm_struct {
    struct vm_area_struct *mmap;
    pagetable_t pagetable;
};

struct vm_area_struct {
    unsigned long vm_start;
    unsigned long vm_end;
    struct mm_struct *vm_mm;
    struct vm_area_struct *vm_next, *vm_prev;
    unsigned long vm_pgoff;
    unsigned long vm_flags;
};

enum zone_type{
    ZONE_NORMAL = 0,
    MAX_NR_ZONES,
};

struct free_area {
    struct list_head free_list;
    unsigned long nr_free;
};

struct pg_data;
struct zone {
    struct pg_data *zone_pgdata;
    unsigned long zone_start_pfn;
    unsigned long zone_end_pfn;
    unsigned long spanned_pages;
    const char *name;
    struct free_area free_area[MAX_ORDER];
};

struct pg_data {
    struct zone node_zones[MAX_NR_ZONES];
    unsigned long node_start_pfn;
    unsigned long node_spanned_pages;
    int node_id;
    struct page *node_mem_map;
};

enum {
    PG_locked,
    PG_reserved,
    PG_buddy,
    PG_slab,
};

struct page {
    unsigned long flags;
    struct list_head lru;
    unsigned long private;
    unsigned int zone_id;
    unsigned int node_id;
    atomic_t refcount;
};

#define PageReserved(page)  get_bit(PG_reserved, &(page)->flags)
#define SetPageReserved(page)	set_bit(PG_reserved, &(page)->flags)
#define ClearPageReserved(page)	clear_bit(PG_reserved, &(page)->flags)

#define PageLocked(page) get_bit(PG_locked, &(page)->flags)
#define LockPage(page)	set_bit(PG_locked, &(page)->flags)

#define PageBuddy(page)  get_bit(PG_buddy, &(page)->flags)
#define SetPageBuddy(page) set_bit(PG_buddy, &(page)->flags)
#define ClearPageBuddy(page) clear_bit(PG_buddy, &(page)->flags)

#define PageSlab(page)  get_bit(PG_slab, &(page)->flags)
#define SetPageSlab(page) set_bit(PG_slab, &(page)->flags)
#define ClearPageSlab(page) clear_bit(PG_slab, &(page)->flags)

extern struct pg_data contig_page_data;
#define NODE_DATA(nid) (&contig_page_data)

static inline struct zone *page_zone(const struct page *page)
{
    return &NODE_DATA(page_to_nid(page))->node_zones[page->zone_id];
}

void mem_init(void);
#endif
