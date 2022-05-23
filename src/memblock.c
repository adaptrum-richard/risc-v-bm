#include "memblock.h"
#include "types.h"
#include "riscv.h"
#include "errorno.h"
#include "printk.h"

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
