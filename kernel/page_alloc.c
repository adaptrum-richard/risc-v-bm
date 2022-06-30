#include "mm.h"
#include "page.h"
#include "list.h"
#include "types.h"
#include "errorno.h"
#include "printk.h"
#include "memblock.h"
#include "string.h"

struct page *mem_map;
struct pg_data contig_page_data;

static char *const zone_names[MAX_NR_ZONES] = {
    "Noraml",
};


static void zone_init_free_lists(struct zone *zone)
{
    unsigned int order;
    for (order = 0; order < MAX_ORDER; order++) {
        INIT_LIST_HEAD(&zone->free_area[order].free_list);
        zone->free_area[order].nr_free = 0;
    }
}

static void init_zone_free_area_list(struct pg_data *pgdat)
{
    struct zone *zone;
    for (int i = 0; i < MAX_NR_ZONES; i++) {
        zone = &pgdat->node_zones[i];
        zone_init_free_lists(zone);
        zone->name = zone_names[i];
    }
}

static void calculate_node_totalpages(struct pg_data *pgdat,
            unsigned long node_start_pfn, unsigned long *zone_size)
{
    enum zone_type i;
    unsigned long zone_start = node_start_pfn;
    unsigned long totalpages = 0;
    for (i = 0; i < MAX_NR_ZONES; i++) {
        struct zone *zone = pgdat->node_zones + i;
        unsigned long size = zone_size[i];

        zone->zone_start_pfn = zone_start;
        zone->spanned_pages = size;

        totalpages += size;
    }
    pgdat->node_spanned_pages = totalpages;
}

static int alloc_node_mem_map(struct pg_data *pgdat)
{
    unsigned long start = 0;
    unsigned long offset = 0;
    unsigned long end;
    unsigned long size;
    struct page *map;

    start = pgdat->node_start_pfn & ~(MAX_ORDER_NR_PAGES - 1);
    offset = pgdat->node_start_pfn - start;

    if (!pgdat->node_mem_map) {
        end = pgdat->node_start_pfn + pgdat->node_spanned_pages;
        end = ALIGN(end, MAX_ORDER_NR_PAGES);
        size =  (end - start) * sizeof(struct page);
        /*为mem_map分配一段内存*/
        map = (struct page *)memblock_virt_alloc(size);
        if (!map)
            return -EINVAL;

        memset(map, 0, size);

        pgdat->node_mem_map = map + offset;
    }
    printk("mem_map:0x%lx, size:%d (%d pages), offset:%d\n",
                    (unsigned long)pgdat->node_mem_map,
                    size, (end - start) / sizeof(struct page), offset);
    mem_map = pgdat->node_mem_map;
    return 0;
}

void free_area_init_node(int nid, unsigned long node_start_pfn,
        unsigned long *zone_size)
{
    struct pg_data *pgdat = NODE_DATA(nid);
    unsigned long start_pfn = 0;
    pgdat->node_id = nid;
    pgdat->node_start_pfn = node_start_pfn;
    start_pfn = node_start_pfn;
    /*初始化struct pg_data*/
    calculate_node_totalpages(pgdat, start_pfn, zone_size);
    /*初始化struct pg_data中的每个zone*/
    init_zone_free_area_list(pgdat);
    /*从memblock中分配mem_map的内存*/
    alloc_node_mem_map(pgdat);
}

void clear_page_order(struct page *page)
{
    page->private = 0;
    ClearPageBuddy(page);
}

static int page_is_buddy(struct page *page, unsigned int order)
{
    if (PageBuddy(page) && page->private == order)
        return 1;
    return 0;
}

void set_page_order(struct page *page, unsigned int order)
{
    page->private = order;
    SetPageBuddy(page);
}

static unsigned long find_buddy_pfn(unsigned long pfn, unsigned int order)
{
    return pfn ^ (1 << order);
}

static void __free_one_page(struct page *page, unsigned long pfn,
		struct zone *zone, unsigned int order)
{
    unsigned int max_order;
    unsigned long buddy_pfn;
    unsigned long combined_pfn;
    struct page *buddy;

    max_order = MAX_ORDER;

    while (order < max_order - 1) {
        /* 往高地址处 找order相同的伙伴*/
        buddy_pfn = find_buddy_pfn(pfn, order);
        /* 伙伴的struct page */
        buddy = page + (buddy_pfn - pfn);

        /* 如果这个伙伴 不是空闲的,
        * 那么直接退出查找
        */
        if (!page_is_buddy(buddy, order))
            goto done_merge;

        /* 找到空闲的伙伴，先从链表中摘下来*/
        list_del(&buddy->lru);
        zone->free_area[order].nr_free--;
        clear_page_order(page);

        /* 继续查找 更大一级oder*/
        combined_pfn = buddy_pfn & pfn;
        page = page + (combined_pfn - pfn);
        pfn = combined_pfn;
        order++;
    }

    /* 把找到的空闲页面添加到伙伴系统中
    * 这时候可能合并了几轮合适的伙伴
    */
done_merge:
    set_page_order(page, order);
    list_add(&page->lru, &zone->free_area[order].free_list);
    zone->free_area[order].nr_free++;
}

static void free_pages_ok(struct page *page, unsigned int order)
{
    unsigned long pfn = page_to_pfn(page);
    __free_one_page(page, pfn, page_zone(page), order);
}

void __free_pages(struct page *page, unsigned int order)
{
    /* 当refcount减1等于0说明这个页面可以被释放 */
    if (atomic_sub_return(1, &page->refcount) == 0){
        free_pages_ok(page, order);
    }
}

static inline void set_page_count(struct page *page, int v)
{
    atomic_set(&page->refcount, v);
}

void memblock_free_pages(struct page *page, unsigned int order)
{
    unsigned int nr_pages = 1 << order;
    struct page *p = page;
    unsigned int loop;
    for (loop = 0; loop < (nr_pages - 1); loop++, p++) {
        ClearPageBuddy(p);
        set_page_count(p, 0);
    }
    set_page_count(page, 1);
    __free_pages(page, order);
}

/*分配内存*/

static void prev_new_page(struct page *page, unsigned int order)
{
    page->private = order;
    set_page_count(page, 1);
}

static void expend(struct zone *zone, struct page *page,
        int low, int high, struct free_area *area)
{
    unsigned long size = 1 << high;
    while (high > low) {
        area--;
        high--;
        size >>= 1;
        list_add(&page[size].lru, &area->free_list);
        area->nr_free++;
        set_page_order(&page[size], high);
    }
}

static struct page *__rmqueue(struct zone *zone, unsigned int order)
{
    unsigned int current_order;
    struct free_area *area;
    struct page *page;

    for (current_order = order; current_order < MAX_ORDER; 
                ++current_order){

        area = &zone->free_area[current_order];
        if (list_empty(&area->free_list))
            continue;
        page = list_entry(area->free_list.next, struct page, lru);
        list_del(&page->lru);
        clear_page_order(page);
        area->nr_free--;
        /* 切大块的空闲内存块 */
        expend(zone, page, order, current_order, area);
        return page;
    }
    return NULL;
}

struct page *alloc_pages(unsigned int order)
{
    struct pg_data *pgdat = NODE_DATA(0);
    struct zone *zone;
    struct page *page;
    for (int i = 0; i < MAX_NR_ZONES; i++) {
        zone = &pgdat->node_zones[i];
        page = __rmqueue(zone, order);
        if (page) {
            prev_new_page(page, order);
            return page;
        }
    }
    return NULL;
}

unsigned long get_free_pages(unsigned int order)
{
    struct page *page;
    page = alloc_pages(order);
    if (!page)
        return 0;

    return (unsigned long)page_to_address(page);
}

unsigned long get_free_page(void)
{
    return get_free_pages(0);
}

void free_pages(unsigned long addr, unsigned int order)
{
    struct page *page;
    page = virt_to_page(addr);
    __free_pages(page, order);
}

void free_page(unsigned long addr)
{
    free_pages(addr, 0);
}

void show_buddyinfo(void)
{
    int order;
    struct zone *zone;
    struct pg_data *pgdat = NODE_DATA(0);
    int i;
    unsigned long pageblock = 1 << (MAX_ORDER - 1);

    printk("Max order:%d, pageblock: %dMB\n",
                MAX_ORDER - 1, (pageblock * 4)/1024);

    printk("%-22s", "Free pages at order");

    for (order = 0; order < MAX_ORDER; ++order)
        printk("%6ld", order);
    printk("\n");

    for (i = 0; i < MAX_NR_ZONES; i++) {
        zone = &pgdat->node_zones[i];
        printk("Node %2d, zone %8s", pgdat->node_id,
                zone->name);
        for (order = 0; order < MAX_ORDER; ++order) {
            unsigned long freecount = 0;
            struct free_area *area;
            struct list_head *p;

            area = &(zone->free_area[order]);
            list_for_each(p, &area->free_list)
                freecount++;

            printk("%6lu", freecount);
        }
    }
    printk("\r\n");
}


