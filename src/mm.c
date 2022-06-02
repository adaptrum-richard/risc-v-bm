#include "mm.h"
#include "memblock.h"
#include "page.h"
#include "memlayout.h"
#include "printk.h"
#include "slab.h"
#include "errorno.h"
#include "proc.h"
#include "string.h"
#include "vm.h"

static void zone_sizes_init(unsigned long min, unsigned long max)
{
    unsigned long zone_size[MAX_NR_ZONES];
    zone_size[ZONE_NORMAL] = max - min;
    free_area_init_node(0, min, zone_size);
}

unsigned long bootmem_get_start_ddr(void)
{
    return KERNBASE;
}

unsigned long bootmem_get_end_ddr(void)
{
    return PHYSTOP;
}

void bootmem_init(void)
{
    unsigned long min, max;
    min = PFN_UP(bootmem_get_start_ddr());
    max = PFN_DOWN(bootmem_get_end_ddr());
    zone_sizes_init(min, max);
    memblock_dump_region();
}


extern char _text_boot[], _end[];
void memblock_init(void)
{
    unsigned long free;
    unsigned long kernel_size;
    unsigned long start_mem, end_mem;
    unsigned long kernel_start, kernel_end;

    start_mem = bootmem_get_start_ddr();
    end_mem = bootmem_get_end_ddr();

    start_mem = PAGE_ALIGN(start_mem);
    end_mem &= PAGE_MASK;
    free = end_mem - start_mem;

    /*TODO:需要实现__pa_symbol*/
    kernel_start =(unsigned long)_text_boot;
    kernel_end = (unsigned long)_end;

    memblock_add_region(start_mem, free);

    kernel_size = kernel_end - kernel_start;

    printk("kernel image: 0x%x - 0x%x, %d bytes\n", 
        kernel_start, kernel_end, kernel_size);

    memblock_reserve(kernel_start, kernel_size);

    free -= kernel_size;

    printk("Memory: %uKB available, %u free pages, kernel_size: %uKB\n",
            free/1024, free/PGSIZE, kernel_size/1024);
}

void mem_init(void)
{
    memblock_init();
    bootmem_init();
    memblock_free_all();
    kmem_cache_init();
}

static inline int mm_alloc_pgd(struct mm_struct *mm)
{
    mm->pagetable = copy_kernel_tbl();
    if(!mm->pagetable)
        return -ENOMEM;
    return 0;
}

static struct mm_struct *mm_init(struct mm_struct *mm, 
    struct task_struct *p)
{
    mm->mmap = NULL;
    if(mm_alloc_pgd(mm))
        return NULL;
    p->mm = mm;
    return mm;
}

/*分配mm，和pgd页面*/
struct mm_struct *mm_alloc(void)
{
    struct mm_struct *mm;
    mm = kmalloc(sizeof(*mm));
    if(!mm){
        return NULL;
    }
    memset(mm, 0, sizeof(*mm));
    return mm_init(mm, current);
}

