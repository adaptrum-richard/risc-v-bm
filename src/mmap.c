#include "mm.h"
#include "mmap.h"
#include "errorno.h"
#include "slab.h"
#include "string.h"
#include "page.h"
#include "types.h"

/*
满足启动一个即可
1.addr在VMA空间范围内，即vma->vm_start <= addr < vma->vm_end。[vma->vm_start, vma->vm_end)
2.距离addr最近并且VMA的结束地址大于addr的一个VMA。
我马上会这么奇怪，要找最近的vma？
原因是在做栈扩展的时候，addr没有落在对应的vma中，则需要做栈的expand(扩展)，不然也没法操作。
参考：https://github.com/torvalds/linux/blob/v5.18/arch/riscv/mm/fault.c#L292
*/

struct vm_area_struct *find_vma(struct mm_struct *mm, unsigned long addr)
{
    struct vm_area_struct *vma;
    struct vm_area_struct *prev;
    /*[ vma )*/
    for(vma = mm->mmap; vma; vma = vma->vm_next){
        if(vma->vm_start <= addr && vma->vm_end > addr)
            return vma;
        if(!vma->vm_prev && addr < vma->vm_start)
            return vma;
        if(vma->vm_prev){
            prev = vma->vm_prev;
            if(prev->vm_end <= addr && vma->vm_end > addr)
                return vma;
        }
    }
    return NULL;
}

//添加
int insert_vm_struct(struct mm_struct *mm, struct vm_area_struct *vma)
{
    struct vm_area_struct *v;
    struct vm_area_struct *next;
    struct vm_area_struct *prev;
    if(!mm->mmap){
        mm->mmap = vma;
        return 0;
    }

    if(mm->mmap->vm_start >= vma->vm_end)
        mm->mmap = vma;

    /*从小到大*/
    for(v = mm->mmap; v; v = v->vm_next){
        /*
         *case1:  prev | vma  | v |  边界可以相等
         */
        if(vma->vm_end <= v->vm_start){
            prev = v->vm_prev;
            if(prev && prev->vm_end > vma->vm_start)
                continue;
            vma->vm_next = v ;
            vma->vm_prev = prev;
            v->vm_prev = vma;
            if(prev)
                prev->vm_next = vma;
            return 0;
        } else{
            /*
             *case2: |v|vma|next
            */
            next = v->vm_next;
            if(next && vma->vm_end > next->vm_start){
                //有重叠，继续找
                continue;
            }
            /*成功，插入，返回*/
            v->vm_next = vma;
            vma->vm_prev = v;
            vma->vm_next = next;
            if(next)
                next->vm_prev = vma;
            return 0;
        }
    }

    return -ENOMEM;
}


static void vma_init(struct vm_area_struct *vma, struct mm_struct *mm)
{
    memset(vma, 0 , sizeof(*vma));
    vma->vm_mm = mm;
}

void vm_area_free(struct vm_area_struct *vma)
{
    kfree(vma);
}

struct vm_area_struct *vm_area_alloc(struct mm_struct *mm)
{
    struct vm_area_struct *vma;
    vma = kmalloc(sizeof(*vma));
    if(vma)
        vma_init(vma, mm);
    return vma;
}

struct vm_area_struct *remove_vma(struct vm_area_struct *vma)
{
    struct vm_area_struct *next = vma->vm_next;
    vm_area_free(vma);
    return next;
}

int expand_downwards(struct vm_area_struct *vma, unsigned long address)
{
    address &= PAGE_MASK;
    if(address < STACK_BOTTOM)
        return -EPERM;
    
    if(address < vma->vm_start){
        unsigned long grow;
        grow = (vma->vm_start - address) >> PGSHIFT;/*计算需要增长的页面数量*/
        if(grow <= vma->vm_pgoff){
            vma->vm_start = address;//扩充栈
            vma->vm_pgoff -= grow;
        }
    }
    //TODO
    return 0;
}

int expand_stack(struct vm_area_struct *vma, unsigned long address)
{
    return expand_downwards(vma, address);
}
