#include "mm.h"
#include "mmap.h"
#include "errorno.h"
#include "slab.h"
#include "string.h"
#include "page.h"
#include "types.h"
#include "proc.h"
#include "vm.h"
#include "printk.h"
#include "debug.h"
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

/*找到[start_addr, end_addr)对应的vma*/
static struct vm_area_struct *find_vma_intersection(struct mm_struct *mm, 
    unsigned long start_addr, unsigned long end_addr)
{
    struct vm_area_struct *vma = find_vma(mm, start_addr);
    if (vma && end_addr <= vma->vm_start)
        vma = NULL;
    return vma;
}

void show_all_vma(struct vm_area_struct *head, const char*info)
{
    struct vm_area_struct *tmp;
    pr_mm_debug("%s", info);
    for(tmp = head;tmp; tmp=tmp->vm_next){
        pr_mm_debug("vm[%lx-%lx]\n", tmp->vm_start, tmp->vm_end);
    }
}

//添加
int insert_vm_struct(struct mm_struct *mm, struct vm_area_struct *vma)
{
    struct vm_area_struct *v;
    struct vm_area_struct *next;
    struct vm_area_struct *prev;
    if(!mm->mmap){
        pr_mm_debug("insert vma addr:0x%lx, va size [0x%lx-0x%lx]\n", 
            (unsigned long)vma, vma->vm_start, vma->vm_end);
        mm->mmap = vma;
        vma->vm_prev = vma->vm_next = NULL;
        return 0;
    }
    show_all_vma(mm->mmap, "insert vma start show:\n");
    /*从小到大*/
    for(v = mm->mmap; v; v = v->vm_next){
        /*
         *case1:  prev | vma  | v |  边界可以相等
         */
        
        if(vma->vm_end <= v->vm_start){
            pr_mm_debug("vma [0x%lx-0x%lx], v[%lx-%lx]\n", vma->vm_start, vma->vm_end, 
                v->vm_start, v->vm_end);
            prev = v->vm_prev;
            if(prev && prev->vm_end > vma->vm_start){
                pr_mm_debug("vma [0x%lx-0x%lx], prev vma[%lx-%lx]\n", vma->vm_start, vma->vm_end, 
                    prev->vm_start, prev->vm_end);
                continue;
            }
            vma->vm_next = v ;
            vma->vm_prev = prev;
            v->vm_prev = vma;
            if(prev)
                prev->vm_next = vma;
            else
                mm->mmap = vma;
            pr_mm_debug("insert vma addr:0x%lx, va size [0x%lx-0x%lx]\n", 
                (unsigned long)vma, vma->vm_start, vma->vm_end);
            show_all_vma(mm->mmap, "insert vma end show:\n");
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
            pr_mm_debug("insert vma addr:0x%lx, va size [0x%lx-0x%lx]\n", 
                (unsigned long)vma, vma->vm_start, vma->vm_end);
            show_all_vma(mm->mmap, "insert vma end show:\n");
            return 0;
        }
    }
    pr_mm_err("error\n");
    return -ENOMEM;
}


static void vma_init(struct vm_area_struct *vma, struct mm_struct *mm)
{
    memset(vma, 0 , sizeof(*vma));
    vma->vm_next = vma->vm_prev = NULL;
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

void vm_area_init(struct vm_area_struct *vma, unsigned long start, 
    unsigned long end, unsigned long flags)
{
    vma->vm_start = start;
    vma->vm_end = end;
    vma->vm_flags = flags;
}

struct vm_area_struct *remove_vma(struct vm_area_struct *vma)
{
    struct vm_area_struct *next = vma->vm_next;
    if(!vma->vm_prev){
        current->mm->mmap = vma->vm_next;
        if(next)
            next->vm_prev = NULL;
    }
    else{
        vma->vm_prev->vm_next = vma->vm_next;
        vma->vm_next->vm_prev = vma->vm_prev;
    }
    vm_area_free(vma);
    return next;
}

int expand_downwards(struct vm_area_struct *vma, unsigned long address)
{
    address &= PAGE_MASK;
    if(address < STACK_BOTTOM)
        return -EPERM;
    
    if(address < vma->vm_start){
        vma->vm_start = PGROUNDDOWN(address);//扩充栈
    }
    //TODO
    return 0;
}

int expand_stack(struct vm_area_struct *vma, unsigned long address)
{
    return expand_downwards(vma, address);
}

static void unmap_region(struct mm_struct *mm, struct vm_area_struct *vma)
{
    unsigned long va_start = vma->vm_start;
    unsigned long npages = (vma->vm_end - va_start)/PGSIZE;
    unmap_validpages(current->mm->pagetable, va_start, npages);
}

/*删除从start-end的vma,并释放物理内存*/
void remove_vma_region(struct vm_area_struct *head, unsigned long start, unsigned long end)
{
    struct vm_area_struct *tmp = head;
    if(start >= end) {
        pr_err("bug\n");
    }
    for(tmp = head; tmp; tmp= tmp->vm_next){
        if(tmp->vm_start >= start && tmp->vm_end <= end){
            pr_mm_debug("todo unmap and remove vma:[0x%lx-0x%lx]\n", tmp->vm_start, tmp->vm_end);
            unmap_region(NULL, tmp);
            remove_vma(tmp);
        }
        if(tmp == head)
            break;
    }
}

/*
 * 断开映射
 * start为新的brk地址
 * len需要释放的内存大小
*/
int __do_munmap(struct mm_struct *mm, unsigned long start, size_t len, bool downgrade)
{
    unsigned long end;
    struct vm_area_struct *vma;
    struct vm_area_struct tmp;
    if(start > USER_MEM_END || len > (USER_MEM_END - start) 
        || USER_MEM_START > start)
        return -EINVAL;
    len = PAGE_ALIGN(len);
    end = start + len;
    pr_mm_debug("maybe unmap mem [0x%lx-0x%lx]\n", start, end);
    if(len == 0)
        return -EINVAL;
    vma = find_vma_intersection(mm, start, end);
    if(!vma){ /*没有找到[start, end)虚拟内存对应的vma，什么都不做*/
        return 0;
    }

    /*用户传入的参数有错误*/
    if(vma->vm_start > start){
        pr_err("args error\n");
        return -EINVAL;
    }

    //释放掉多余的内存
    tmp.vm_start = start;
    tmp.vm_end = end;
    unmap_region(mm, &tmp);
    pr_mm_debug("determine unmap mem [0x%lx-0x%lx]\n", tmp.vm_start, tmp.vm_end);
    if(mm->start_brk == start ){
        pr_mm_debug("remove vma [0x%lx-0x%lx]\n", vma->vm_start, vma->vm_end);
        remove_vma(vma);
    }
    else{
        if(vma->vm_end < start){
            pr_mm_err("bug\n");
        }
        vma->vm_end = start;

    }
    return 0;
}


/*
addr: 旧的brk地址
len:需要分配的内存大小
*/
static int do_brk_flags(unsigned long addr, unsigned long len, unsigned long flags)
{
    struct mm_struct *mm = current->mm;
    struct vm_area_struct *vma, *newvma = NULL;
    pgoff_t pgoff = addr >> PGSHIFT;
    unsigned long end = addr + PGROUNDUP(len);
    
    //找到brk对应的vma
    vma = find_vma(mm, mm->start_brk);
    
    /*1.当vma为空，则需要新增一个，当找到的vma->vm_start大于等于end地址时，也需要新增一个*/
    if(!vma || vma->vm_start > end){
        newvma = vm_area_alloc(mm);
        /*TODO:检查返回值*/
        newvma->vm_start = addr,
        newvma->vm_end = end;
        newvma->vm_pgoff = pgoff;
        newvma->vm_flags = VM_DATA_FLAGS_NON_EXEC;
        insert_vm_struct(mm, newvma);
    }else{
        /*对vma进行扩容*/
        if(vma->vm_end <= end){
            vma->vm_end = end;
        }
        else{
            //此时vma已经有了，不需要设置
            pr_err("%s %d: dothing\n", __func__, __LINE__);
        }
    }

    return 0;
}

/*free和malloc均会调用do_brk
每个进程都有一个mm->brk边界。
当brk小于mm->brk表示需要释放内存。
当brk大于mm->brk表示需要分配内存。
会不会出现brk一直增大的问题？
会，但是操作系统中进程的堆空间会很大，即使brk在一直增大，总会有释放的情况，所以不会出现brk
大于堆空间的问题。可以放心使用。
*/
static unsigned long do_brk(unsigned long brk)
{
    unsigned long newbrk, oldbrk, origbrk;
    struct mm_struct *mm = current->mm;
    unsigned long min_brk = mm->start_brk;
    
    origbrk = mm->brk;
    if(brk < min_brk)
        goto out;

    newbrk = PGROUNDDOWN(brk);
    oldbrk = mm->brk;
    if(oldbrk == newbrk){
        mm->brk = brk;
        goto success;
    }
    /*当参数brk地址小于当前brk时，需要释放内存*/
    if(brk < mm->brk){
        int ret;
        mm->brk = newbrk; //至少要释放一个page的内存
        /*释放内存*/
        ret = __do_munmap(mm, newbrk, oldbrk-newbrk, true);
        if(ret < 0){
            mm->brk = origbrk;
            goto out;
        }
        goto success;
    }
    
    if(do_brk_flags(oldbrk, PGROUNDUP(brk) - oldbrk, 0) < 0)
        goto out;

    mm->brk = brk;
success:
    /*TODO: 没有实现内存lock，即立即分配物理内存，建立映射*/
    return brk;
out:
    return origbrk;
}

unsigned long sbrk(unsigned long size)
{
    unsigned long new_brk = 0;
    unsigned long old_brk = current->mm->brk;
    if(size == 0)
        return old_brk;
    else {
        new_brk = do_brk(current->mm->brk + PGROUNDUP(size));
        pr_mm_debug("old_brk:0x%lx, new_brk:0x%lx\n", old_brk, new_brk);
        if(old_brk == new_brk)
            return 0;
        else 
            return new_brk - PGROUNDUP(size);
    }
}

unsigned long brk(unsigned long addr)
{
    unsigned long old_brk = current->mm->brk;
    unsigned long new_brk = do_brk(addr);
    pr_mm_debug("old_brk:0x%lx, new_brk:0x%lx\n", old_brk, new_brk);
    return old_brk == new_brk;
}

void show_vma_info(pagetable_t pagatable, struct vm_area_struct *vma)
{
    struct vm_area_struct *tmp;
    uint64 pa;
    //pte_t *walk(pagetable_t pagetable, uint64 va, int alloc)
    if(!pagatable || !vma)
        return;
    pr_mm_debug("-------------show vma, mmap pages-----------------------------\n");
    for(tmp = vma; tmp; tmp = tmp->vm_next){
        pr_mm_debug("[0x%lx -- 0x%lx], flag 0x%lx, mapping pages:\n", tmp->vm_start, tmp->vm_end, tmp->vm_flags);
        for(int i = 0; i < PGROUNDUP(tmp->vm_end - tmp->vm_start); i+= PGSIZE){
            pa = walkaddr(pagatable, tmp->vm_start + i); 
            if(pa){
                pr_mm_debug("\tpage phy addr: 0x%lx\n", pa);
            }
        }
    }
    pr_mm_debug("----------------------------------------------------------------\n");

}
