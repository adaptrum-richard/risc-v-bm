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

// Remove npages of mappings starting from va. va must be
// page-aligned. The mappings must exist.
// Optionally free the physical memory.
void uvmunmap(pagetable_t pagetable, uint64 va, uint64 npages, int do_free)
{
    uint64 a;
    pte_t *pte;

    if((va % PGSIZE) != 0)
        panic("uvmunmap: not aligned");

    for(a = va; a < va + npages*PGSIZE; a += PGSIZE){
        if((pte = walk(pagetable, a, 0)) == 0)
            panic("uvmunmap: walk");
        if((*pte & PTE_V) == 0)
            panic("uvmunmap: not mapped");
        if(PTE_FLAGS(*pte) == PTE_V)
            panic("uvmunmap: not a leaf");
        if(do_free){
            uint64 pa = PTE2PA(*pte);
            free_page((unsigned long)pa);
        }
        *pte = 0;
    }
}

void unmap_validpages(pagetable_t pagetable, uint64 va, uint64 npages)
{
    uint64 a;
    pte_t *pte;

    if((va % PGSIZE) != 0)
        panic("uvmunmap: not aligned");

    for(a = va; a < va + npages*PGSIZE; a += PGSIZE){
        if((pte = walk(pagetable, a, 0)) == 0)
            panic("uvmunmap: walk");
        if((*pte & PTE_V) == 0)
            continue;
        if(PTE_FLAGS(*pte) == PTE_V)
            continue;
        
        uint64 pa = PTE2PA(*pte);
        free_page((unsigned long)pa);
        *pte = 0;
    }
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
    mappages(pagetable, USER_CODE_VM_START, PGROUNDUP(size), (uint64)mem, PTE_W|PTE_R|PTE_X|PTE_U);
    memmove(mem, src, size);
    return 0;
}

/*根据虚拟地址找到对应的物理地址
 *返回0，表示此虚拟地址没有被映射
 *此函数仅仅能被用于查找用户空间的页
 */
uint64 walkaddr(pagetable_t pagetable, uint64 va)
{
    pte_t *pte;
    uint64 pa;

    if(va >= MAXVA)
        return 0;

    pte = walk(pagetable, va, 0);
    if(pte == 0)
        return 0;
    if((*pte & PTE_V) == 0)
        return 0;
    if((*pte & PTE_U) == 0)
        return 0;
    pa = PTE2PA(*pte);
    return pa;
}

static int copy_data_kernel_to_user(pagetable_t pagetable, uint64 dstva, char *src, uint64 len)
{
    uint64 n, va0, pa0;
    while(len > 0){
        va0 = PGROUNDDOWN(dstva);
        pa0 = walkaddr(pagetable, va0);
        if(pa0 == 0)
            return -1;
        n = PGSIZE - (dstva - va0);
        if(n > len)
            memmove((void*)(pa0 + (dstva - va0)), src, n);
        len -= n;
        src += n;
        dstva = va0 + PGSIZE; 
    }
    return 0;
}

static int copy_data_user_to_kernel(pagetable_t pagetable, char *dst, uint64 srcva, uint64 len)
{
    uint64 n, va0, pa0;

    while(len > 0){
        va0 = PGROUNDDOWN(srcva);
        pa0 = walkaddr(pagetable, va0);
        if(pa0 == 0)
            return -1;
        n = PGSIZE - (srcva - va0);
        if(n > len)
            n = len;
        memmove(dst, (void *)(pa0 + (srcva - va0)), n);

        len -= n;
        dst += n;
        srcva = va0 + PGSIZE;
    }
    return 0;
}

static int user_space_address(uint64 addr)
{
    if(addr >= USER_CODE_VM_START)
        return 1;
    return 0;
}

/*如果user dst为1，表示将数据复制到用户空间内存中，否则是内核中的操作
找到dst_va虚拟地址对应的物理地址，将数据src_data_addr复制到物理地址中，长度为len
*/
int space_data_copy_out(uint64 dst_va, void *src_data_addr, uint len)
{
    int user_dst = user_space_address(dst_va);
    if(user_dst){
        return copy_data_kernel_to_user(current->mm->pagetable, dst_va, src_data_addr, len);
    } else {
        memmove((char *)dst_va, src_data_addr, len);
        return 0;
    }
}

/*如果user_src为1，表示将数据复制到用户空间内存中，否则为内存空间
找到src_va对应的物理地址，将物理地址中的数据复制到dst_data_addr中。
*/
int spcae_data_copy_in(void *dst_data_addr, uint64 src_va, uint64 len)
{
    int user_src = user_space_address(src_va);
    if(user_src){
        return copy_data_user_to_kernel(current->mm->pagetable, dst_data_addr, src_va, len);
    } else {
        memmove(dst_data_addr, (char*)src_va, len);
        return 0;
    }
}
