#include "page.h"
#include "vm.h"
#include "riscv.h"
#include "string.h"
#include "printk.h"
#include "memlayout.h"
#include "page.h"
#include "errorno.h"
#include "proc.h"

pagetable_t kernel_pagetable;
extern char _etext[];

// Return the address of the PTE in page table pagetable
// that corresponds to virtual address va.  If alloc!=0,
// create any required page-table pages.
//
// The risc-v Sv39 scheme has three levels of page-table
// pages. A page-table page contains 512 64-bit PTEs.
// A 64-bit virtual address is split into five fields:
//   39..63 -- must be zero.
//   30..38 -- 9 bits of level-2 index.
//   21..29 -- 9 bits of level-1 index.
//   12..20 -- 9 bits of level-0 index.
//    0..11 -- 12 bits of byte offset within the page.
pte_t *walk(pagetable_t pagetable, uint64 va, int alloc)
{
    if (va >= MAXVA){
        panic("walk");
    }

    for (int level = 2; level > 0; level--){
        pte_t *pte = &pagetable[PX(level, va)];
        if (*pte & PTE_V){
            pagetable = (pagetable_t)PTE2PA(*pte);
        } else {
            if (!alloc || (pagetable = (pte_t *)get_free_page()) == 0)
                return 0;
            memset(pagetable, 0, PGSIZE);
            *pte = PA2PTE(pagetable) | PTE_V;
        }
    }
    return &pagetable[PX(0, va)];
}

int mappages(pagetable_t pagetable, uint64 va, uint64 size, uint64 pa, int perm)
{
    uint64 a, last;
    pte_t *pte;
    if (!size) {
        panic("mappages: size");
    }
    a = PGROUNDDOWN(va);
    last = PGROUNDDOWN(va + size - 1);
    for (;;) {
        if ((pte = walk(pagetable, a, 1)) == 0)
            return -1;
        if (*pte & PTE_V)
            panic("mappages: remap");
        *pte = PA2PTE(pa) | perm | PTE_V;
        if (a == last)
            break;
        a += PGSIZE;
        pa += PGSIZE;
    }
    return 0;
}

void kvmmap(pagetable_t kpgtbl, uint64 va, uint64 pa, uint64 sz, int perm)
{
    if (mappages(kpgtbl, va, sz, pa, perm) != 0)
        panic("kvmmap");
}

pagetable_t kvmmake(void)
{
    pagetable_t kpgtbl;

    kpgtbl = (pagetable_t)get_free_page();
    memset(kpgtbl, 0, PGSIZE);

    // uart registers
    kvmmap(kpgtbl, UART0, UART0, PGSIZE, PTE_R | PTE_W);

    // virtio mmio disk interface
    kvmmap(kpgtbl, VIRTIO0, VIRTIO0, PGSIZE, PTE_R | PTE_W);

    // PLIC
    kvmmap(kpgtbl, PLIC, PLIC, 0x400000, PTE_R | PTE_W);

    // map kernel text executable and read-only.
    kvmmap(kpgtbl, KERNBASE, KERNBASE, (uint64)_etext - KERNBASE, PTE_R | PTE_X);

    // map kernel data and the physical RAM we'll make use of.
    kvmmap(kpgtbl, (uint64)_etext, (uint64)_etext, PHYSTOP - (uint64)_etext, PTE_R | PTE_W);

    // map the trampoline for trap entry/exit to
    // the highest virtual address in the kernel.
    // kvmmap(kpgtbl, TRAMPOLINE, (uint64)trampoline, PGSIZE, PTE_R | PTE_X);

    // map kernel stacks
    // proc_mapstacks(kpgtbl);

    return kpgtbl;
}

void kvminit(void)
{
    kernel_pagetable = kvmmake();
}

void kvminithart()
{
    w_satp(MAKE_SATP(kernel_pagetable));
    sfence_vma();
}

pagetable_t get_kpgtbl()
{
    return kernel_pagetable;
}


/*创建一个空的page table*/
pagetable_t uvmcreate()
{
    pagetable_t pagetable;
    pagetable = (pagetable_t)get_free_page();
    if(pagetable == 0)
        return 0;
    memset(pagetable, 0, PGSIZE);
    return pagetable;
}

pagetable_t copy_kernel_tbl(void)
{
    pagetable_t new = (pagetable_t)get_free_page();
    if(new){
        memset(new, 0, PGSIZE);
        memcpy(new, kernel_pagetable, PGSIZE);
    }
    return new;
}

/*加载user的程序，并映射到0地址*/
int uvminit(pagetable_t pagetable, uchar *src, uint size)
{
    char *mem;
    int order = get_order(PGROUNDUP(size));
    mem = (char*)get_free_pages(order);
    /*TODO: 处理内存不足的情况*/
    if(!mem){
        return -ENOMEM;
    }
    memset(mem, 0, PGROUNDUP(size));
    mappages(pagetable, USER_START, PGROUNDUP(size), (uint64)mem, PTE_W|PTE_R|PTE_X|PTE_U);
    memmove(mem, src, size);
    return 0;
}

int user_stack_growsdown(unsigned long addr)
{
    addr = PAGE_ALIGN(addr) - PGSIZE;
    uint64 mem = (uint64)get_free_page();
    if(!mem){
        return -ENOMEM;
    }
    memset((void *)mem, 0, PGSIZE);
    mappages(current->mm->pagetable, addr, PGSIZE,  mem, PTE_W|PTE_R|PTE_U);
    
    return 0;
}
