#ifndef __MM_H__
#define __MM_H__
#include "riscv.h"
#include "list.h"
#include "atomic.h"
#include "bitops.h"

/*什么情况下，设置了VM_MAY%,而没有设置对应的VM_%属性呢？
在内存为只读的情况下，希望设置写的属性。
例如，COW机制根据VM_MAYWRITE标志来检测受COW保护的页面
当设置VM_MAYWRITE时，Linux内核没有设置
VM_WRITE标志吗?如果是的话，为什么不从一开始就设置一个标志呢?
不，它没有。 is_cow_mapping() 不是检查内存是否可写，而是检查
内存是否可以通过 mprotect() 变为可写。如果不能，那么它显然不是 COW 映射！
https://stackoverflow.com/questions/48241187/memory-region-flags-in-linux-why-both-vm-write-and-vm-maywrite-are-needed
*/
/* mprotect() hardcodes VM_MAYREAD >> 4 == VM_READ, and so for r/w/x bits. */
#define VM_MAYREAD	0x00000010	/* limits for mprotect() etc */
#define VM_MAYWRITE	0x00000020
#define VM_MAYEXEC	0x00000040
#define VM_MAYSHARE	0x00000080

/*vm flags*/
#define VM_NONE		    0x00000000
#define VM_READ		    0x00000001	/* currently active flags */
#define VM_WRITE	    0x00000002
#define VM_EXEC		    0x00000004
#define VM_SHARED	    0x00000008

#define VM_GROWSDOWN	0x00000100	/* general info on the segment */
#define VM_ACCOUNT	    0x00100000	/* Is a VM accounted object */
#define VM_STACK	     VM_GROWSDOWN

#define VM_ACCESS_FLAGS (VM_READ | VM_WRITE | VM_EXEC)

#define MAX_ORDER 11
#define MAX_ORDER_NR_PAGES (1 << (MAX_ORDER - 1))

#define VM_DATA_FLAGS_NON_EXEC	(VM_READ | VM_WRITE | VM_MAYREAD | \
  				 VM_MAYWRITE | VM_MAYEXEC)
#define VM_STACK_DEFAULT_FLAGS VM_DATA_FLAGS_NON_EXEC

#define VM_STACK_FLAGS	(VM_STACK | VM_STACK_DEFAULT_FLAGS | VM_ACCOUNT)

struct vm_area_struct;
struct mm_struct {
    pagetable_t pagetable;
    struct vm_area_struct *mmap;
    unsigned long stack_vm;	   /* VM_STACK */
    unsigned long total_vm;
};

struct vm_area_struct {
    unsigned long vm_start;
    unsigned long vm_end;
    struct mm_struct *vm_mm;
    struct vm_area_struct *vm_next, *vm_prev; /*双向链表，并不是双向循环链表*/
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
    //slob
    void *freelist;
    int slob_left_units;
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
struct mm_struct *mm_alloc(void);
#endif
