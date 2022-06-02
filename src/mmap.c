#include "mm.h"
#include "mmap.h"
#include "errorno.h"

//查找
struct vm_area_struct *find_vma(struct mm_struct *mm, unsigned long addr)
{
    struct vm_area_struct *vma;
    /*[ vma )*/
    for(vma = mm->mmap; vma; vma = vma->vm_next){
        if(vma->vm_start <= addr && vma->vm_end > addr)
            return vma;
        if(addr <  vma->vm_end)
            goto out;        
    }
out:
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
