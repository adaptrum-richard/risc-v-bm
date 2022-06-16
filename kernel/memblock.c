#include "memblock.h"
#include "types.h"
#include "riscv.h"
#include "errorno.h"
#include "printk.h"
#include "page.h"
#include "mm.h"

static struct memblock_region memblock_regions[MAX_MEMBLOCK_REGIONS];

struct memblock memblock = {
    .head.next = NULL,
    .num_regions = 0,
    .total_size = 0,
};

/*linux内核代码结束位置*/
extern char _end[];

static void memblock_insert_new(struct memblock_region *last, struct memblock_region *new,
    struct memblock_region *mrg)
{
    last->next = new;
    new->prev = last;
    new->next = mrg;
    mrg->prev = new;
}

static struct memblock_region *memblock_get_region_entity(void)
{
    int i;
    for(i = 0; i < MAX_MEMBLOCK_REGIONS; i++){
        if(memblock_regions[i].allocated == 0){
            memblock_regions->allocated = 1;
            return &memblock_regions[i];
        }
    }
    return NULL;
}

static void memblock_free_region_entity(struct memblock_region *mrg)
{
    mrg->allocated = 0;
    mrg->next = mrg->prev = NULL;
}

static struct memblock_region *memblock_init_entity(unsigned long base,
        unsigned long size, unsigned int flags)
{
    struct memblock_region *mrg;
    mrg = memblock_get_region_entity();
    if(!mrg)
        return NULL;
    mrg->base = base;
    mrg->size = size;
    mrg->flags = flags;
    mrg->prev = mrg->next = NULL;
    return mrg;
}

int memblock_add_region(unsigned long base, unsigned long size)
{
    struct memblock_region *mrg, *next, *new;
    unsigned long mbase, mend;
    unsigned long end = base + size;
    base = PGROUNDDOWN(base);
    size = PGROUNDUP(size);

    new = memblock_init_entity(base, size, MEMBLOCK_FREE);
    if(!new)
        return -EINVAL;
    
    /*membloc是空的情况*/
    if(memblock.head.next == NULL){
        memblock.head.next = new;
        new->prev = &memblock.head;
        memblock.total_size = size;
        memblock.num_regions++;
        return 0;
    }
    /*从memblock_region.head遍历*/
    for_each_memblock_region(mrg){
        mbase = mrg->base;
        mend = mrg->base + mrg->size;
        next = mrg->next;

/*
小------->>>>-------大
---------------------
    |       |
    V       V
    mend    base
*/
        if(mend < base){
            if(mrg->next)
                continue; /*不是最后一个则继续*/
            else /*表示已经是最后一个*/
                goto found_tail;
        }
/*
小------->>>>-------大
----------------------------
    |       |       |
    V       V       V
    base    mend    end
*/
        if(base < mend && mend < end){
            /*说明有重叠的地方，有bug*/
            printk("%s,%d: memeblock overlap: [0x%lx ~ 0x%lx]\n",
                __func__, __LINE__, mbase, mend);
            break;
        }
/*
小------->>>>-------大
----------------------------------------
    |        |       |           |
    V        V       V           V
    mend   start  next->base     end    
*/
        if(next && next->base < end){
            /*不在两个region中间，有重叠*/
            printk("%s,%d: memeblock overlap: [0x%lx ~ 0x%lx]\n",
                __func__, __LINE__, mbase, mend);
            break;                
        }
        goto found;
    }
    memblock_free_region_entity(new);
found:
    memblock_insert_new(mrg->prev, new, mrg);
found_tail:
    mrg->next = new;
    new->prev = mrg;
    return 0;
}

static void *memblock_alloc(unsigned size)
{
    unsigned long alloc_start;
    struct memblock_region *mrg, *prev;
    unsigned long mbase, mend;
    struct memblock_region *new;
    unsigned long kernel_end = (unsigned long)_end;
    size = PGROUNDUP(size);
    
    /*从内核结束位置开始查找空闲内存*/
    alloc_start = PAGE_ALIGN(kernel_end);
    for_each_memblock_region(mrg){
        mbase = mrg->base;
        mend = mrg->base+mrg->size;
        prev = mrg->prev;

        /*找到一个适合的memblock*/
        if(mrg->flags == MEMBLOCK_RESERVE)
            continue;
        if(mbase < alloc_start || mend <= alloc_start)
            continue;
        if(mrg->size < size)
            continue;
        
        /*当prev的memblock属性是RESERVE, next的属性是FREE，两者相邻，
        那么可以把prev的memblock往上扩展，从而分配空闲页面*/
        if(((prev->base + prev->size) == mbase) 
            && prev->flags == MEMBLOCK_RESERVE 
            && mrg->flags == MEMBLOCK_FREE){
            prev->size += size;
            mrg->base += size;
            mrg->size -= size;
            return (void*)mbase;
        }
        new = memblock_init_entity(mbase, size, MEMBLOCK_RESERVE);
        if(!new)
            return NULL;
        
        mrg->base = mbase + size;
        mrg->size = mrg->size - size;

        memblock_insert_new(prev, new, mrg);
        memblock.num_regions++;
        return (void*)new->base;
    }
    return NULL;
}

int memblock_reserve(unsigned long base, unsigned long size)
{
    struct memblock_region *mrg, *prev;
    unsigned long mbase, mend;
    struct memblock_region *new, *new1;
    unsigned long end = base + size;

    base = PGROUNDDOWN(base);
    size = PGROUNDUP(size);

    for_each_memblock_region(mrg){
        mbase = mrg->base;
        mend = mrg->base + mrg->size;
        prev = mrg->prev;

        /*memblock按照从小到大的顺序排列，根本没有合适的memblock，退出循环*/
        if(mbase > end)
            break;
        
        if(mend < base)
            continue;
        
        if(mrg->flags == MEMBLOCK_RESERVE)
            continue;
        
        /*情况1：刚好完全吻合*/
        if(mend == end && mbase == base){
            mrg->flags = MEMBLOCK_RESERVE;
            return 0;
        } else if(mbase == base){
            /*情况2：低地址吻合*/
            new = memblock_init_entity(base, size, MEMBLOCK_RESERVE);
            if(!new)
                return -EINVAL;
            mrg->base += size;
            mrg->size -= size;
            memblock_insert_new(prev, new, mrg);
            memblock.num_regions++;
            return 0;
        } else if (mend == base + size){
            /*情况3：高地址正好相等，低地址多出一截*/
            new = memblock_init_entity(mbase, mrg->size - size, MEMBLOCK_FREE);
            if(!new)
                return -EINVAL;
            mrg->size -= size;
            mrg->base += size;
            mrg->flags = MEMBLOCK_RESERVE;
            memblock_insert_new(prev, new, mrg);
            memblock.num_regions++;
            return 0;
        } else if(mbase < base && mend > end){
            /*情况4：正好在中间，两头多出一截*/
            new = memblock_init_entity(mbase, base - mbase, MEMBLOCK_FREE);
            if(!new)
                return -EINVAL;
            new1 = memblock_init_entity(base, size, MEMBLOCK_RESERVE);
            if(!new1)
                return -EINVAL;
            mrg->base = base + size;
            mrg->size = mrg->size - new1->size - new->size;
            mrg->flags = MEMBLOCK_FREE;

            memblock_insert_new(prev, new, new1);
            memblock.num_regions++;

            memblock_insert_new(new, new1, mrg);
            memblock.num_regions++;
            return 0;
        }

    }
    return -EINVAL;
}

void *memblock_virt_alloc(unsigned long size)
{
    return memblock_alloc(size);
}

void reserve_mem_region(unsigned long start, unsigned long end)
{
    unsigned long start_pfn = PFN_DOWN(start);
    unsigned long end_pfn = PFN_UP(end);
    for(; start_pfn < end_pfn; start_pfn++){
        struct page *page = pfn_to_page(start_pfn);
        SetPageReserved(page);
    }
}
static unsigned long memblock_free_memory(unsigned long start,
                            unsigned long end)
{
    unsigned long start_pfn = PFN_DOWN(start);
    unsigned long end_pfn = PFN_UP(end);
    unsigned long size = 0;
    int order;

    if (start_pfn >= end_pfn)
        return 0;
    
    size = end - start;
    while(start_pfn < end_pfn){
        order = min(MAX_ORDER - 1,  __ffs(start_pfn));
        while ((start_pfn + (1 << order)) > end_pfn)
            order--;
        memblock_free_pages(pfn_to_page(start_pfn), order);
        start_pfn += (1 << order);
    }
    return size;
}

unsigned long memblock_free_all(void)
{
    struct memblock_region *mrg;
    unsigned long count = 0;
    unsigned long reserve = 0;
    for_each_memblock_region(mrg){
        if(mrg->flags == MEMBLOCK_RESERVE){
            reserve_mem_region(mrg->base, mrg->base + mrg->size);
            reserve += mrg->size;
        }
        if(mrg->flags == MEMBLOCK_FREE){
            count += memblock_free_memory(mrg->base, mrg->base + mrg->size);
        }
    }
    printk("Memory: total %uMB, add %uKB to buddy, %uKB reserve\n",
            (count + reserve)/1024/1024, count/1024, reserve/1024);
    show_buddyinfo();
    return count;
}

void memblock_dump_region(void)
{
    struct memblock_region *mrg;

    printk("dump all memblock regions:\n");

    for_each_memblock_region(mrg) {
        printk("0x%08lx - 0x%08lx, flags: %s\n",
            mrg->base, mrg->base + mrg->size,
            mrg->flags ? "RESERVE":"FREE");
    }
}
