#include "fork.h"
#include "types.h"
#include "page.h"
#include "proc.h"
#include "string.h"
#include "riscv.h"
#include "pt_regs.h"
#include "printk.h"
#include "sched.h"
#include "string.h"
#include "preempt.h"
#include "spinlock.h"
#include "barrier.h"
#include "vm.h"
#include "slab.h"
#include "memlayout.h"
#include "mmap.h"
#include "mm.h"
#include "debug.h"

extern void ret_from_kernel_thread(void);
extern void ret_from_fork(void);

struct vm_area_struct *vm_area_dup(struct vm_area_struct *orig)
{
    struct vm_area_struct *new = kmalloc(sizeof(struct vm_area_struct));
    if(new){
        *new = *orig;
        new->vm_next = new->vm_prev = NULL;
    }
    return new;
}

static int dup_vma_mmap(pagetable_t oldpt, pagetable_t newpt, uint64 oldva, uint64 newva, uint64 sz)
{
    pte_t *pte;
    uint flags;
    uint64 va, pa;
    uint64 mem;
    uint64 newva_bak = newva;
    for(va = oldva; va < oldva + sz; va += PGSIZE, newva += PGSIZE){
        if((pte = walk(oldpt, va, 0)) == 0){
            continue;
            //panic("dup_vma: pte should exist");
        }
        if((*pte & PTE_V) == 0){
            pr_err("va = %lx\n", va);
            panic("dup_vma: page not present");
        }
        pa = PTE2PA(*pte);
        flags = PTE_FLAGS(*pte);
        if((mem = get_free_page()) == 0){
            goto err;  
        }
        memcpy((void*)mem, (void *)pa, PGSIZE);
        if(mappages(newpt, newva, PGSIZE, mem, flags) != 0){
            free_page(mem);
            goto err;
        }
    }
    return 0;
err:
    unmap_validpages(newpt, newva_bak, (newva_bak + sz / PGSIZE));
    return -1;
}

static int dup_mmap(struct mm_struct *mm, struct mm_struct *oldmm)
{
    struct vm_area_struct *oldvma, *newvma;
    mm->total_vm = oldmm->total_vm;
    mm->stack_vm = oldmm->stack_vm;
    mm->start_brk = oldmm->start_brk;
    mm->brk = oldmm->brk;
    for(oldvma = oldmm->mmap; oldvma; oldvma = oldvma->vm_next){
        newvma = vm_area_dup(oldvma);
        insert_vm_struct(mm, newvma);
        dup_vma_mmap(oldmm->pagetable, mm->pagetable, oldvma->vm_start, 
            newvma->vm_start, oldvma->vm_end - oldvma->vm_start);
    }
    return 0;
}

struct mm_struct *dup_mm(struct task_struct *tsk, struct mm_struct *oldmm)
{
    struct mm_struct *mm;
    int err;

    mm = kmalloc(sizeof(*mm));
    if(!mm){
        return NULL;
    }

    memcpy(mm, oldmm, sizeof(*mm));
    if(mm_init(mm, tsk) == NULL){
        kfree(mm);
        return NULL;
    }
    err = dup_mmap(mm, oldmm);
    if(err != 0)
        return NULL;
    return mm;
}

static inline struct pt_regs *task_pt_regs(struct task_struct *tsk)
{
    unsigned long p = (uint64)tsk + THREAD_SIZE - sizeof(struct pt_regs);
    return (struct pt_regs *)p;
}

void print_epc(struct task_struct *tsk)
{
    struct pt_regs *regs = task_pt_regs(tsk);
    printk("epc = %lx\n", regs->epc); 
}

void print_regs_sp(struct task_struct *tsk)
{
    struct pt_regs *regs = task_pt_regs(tsk);
    printk("regs->sp = %lx\n", regs->sp);     
}

void print_regs_tp(struct task_struct *tsk)
{
    struct pt_regs *regs = task_pt_regs(tsk);
    printk("regs->tp = %lx\n", regs->tp);       
}

void set_user_mode_epc(struct task_struct *tsk, uint64 epc)
{
    //返回用户空间执行的address
    struct pt_regs *regs = task_pt_regs(tsk);
    regs->epc = epc; 
}


void set_user_mode_a1(struct task_struct *tsk, uint64 a1)
{
    //返回用户空间执行的address
    struct pt_regs *regs = task_pt_regs(tsk);
    regs->a1 = a1;
}

void set_user_mode_sp(struct task_struct *tsk, uint64 sp)
{
    //返回用户空间执行的address
    struct pt_regs *regs = task_pt_regs(tsk);
    regs->sp = sp;
}

int copy_process(uint64 clone_flags, uint64 fn, uint64 arg, char *name)
{
    int pid = -1;
    unsigned long flags;
    /*内核栈为8K*/
    struct task_struct *p = (struct task_struct *)get_free_pages(get_order(THREAD_SIZE));
    if(!p)
        return -1;
    
    if((pid = get_free_pid()) == -1){
        free_pages(get_order(THREAD_SIZE), (unsigned long)p);
        return -1;
    }
    memset(p, 0, sizeof(*p));
    struct pt_regs *childregs = task_pt_regs(p);
    memset(childregs, 0x0, sizeof(struct pt_regs));
    memset(&p->thread, 0x0, sizeof(p->thread));
    
    if(clone_flags & PF_KTHREAD){
        p->thread.s[0] = fn;
        p->thread.s[1] = arg;
        p->thread.ra = (uint64)ret_from_kernel_thread;
        p->state = TASK_RUNNING;
        p->flags = PF_KTHREAD | TASK_NORMAL;
        /* Supervisor irqs on: */
        childregs->status = SSTATUS_SPP | SSTATUS_SPIE;
    } else {
        //print_all_vma(current->mm->pagetable, current->mm->mmap);
        *childregs = *(task_pt_regs(current));
        childregs->a0 = 0;
        p->thread.ra = (unsigned long)ret_from_fork;
        p->flags = TASK_NORMAL;
        p->state = TASK_RUNNING;
        initlock(&p->wait_childexit.lock, name);
        initlock(&p->lock, name);
        p->wait_childexit.head.next = &(p->wait_childexit.head);
        p->wait_childexit.head.prev = &(p->wait_childexit.head);

        if(dup_mm(p, current->mm) == NULL){
            panic("copy_process: dup_mm failed\n");
        }
        for(int i = 0; i < NOFILE; i++){
            if(current->ofile[i])
                p->ofile[i] = filedup(current->ofile[i]);
        }
        p->cwd = idup(current->cwd);
        //print_all_vma(p->mm->pagetable, p->mm->mmap);
    }
    
    p->priority = current->priority;
    p->counter = DEF_COUNTER;
    p->preempt_count = 0;
    p->pid = pid;
    p->cpu = smp_processor_id();
    set_task_sched_class(p);
    if(name)
        strcpy(p->name, name);
    p->thread.sp = (uint64)childregs;
    p->kernel_sp = p->thread.sp;
    p->parent = current;
    LINK_TASK(p);
    mb();
    spin_lock_irqsave(&(cpu_rq(smp_processor_id())->lock),flags);
    p->sched_class->enqueue_task(cpu_rq(task_cpu(p)), p);
    spin_unlock_irqrestore(&(cpu_rq(smp_processor_id())->lock), flags);
    return pid;
}

int move_to_user_mode(unsigned long start, unsigned long size, 
        unsigned long pc)
{
    struct mm_struct *mm;
    struct vm_area_struct *vma;
    struct pt_regs *regs = task_pt_regs(current);
    memset((char*)regs, 0x0, sizeof(struct pt_regs));
    regs->epc = pc + USER_CODE_VM_START;

    regs->status &= ~SSTATUS_SPP;
    regs->status |= (SSTATUS_SPIE|SSTATUS_SUM);

    regs->sp = STACK_TOP_MAX;

    mm = mm_alloc();
    
    if(map_program_code(mm->pagetable, USER_CODE_VM_START, (uchar*)start, size) < 0){
        printk("%s %d: vm_map_program error\n", __func__, __LINE__);
        return -1;
    }
    vma = vm_area_alloc(mm);
    vma->vm_start = USER_CODE_VM_START;
    vma->vm_end = USER_CODE_VM_START + PGROUNDUP(size);
    vma->vm_flags = VM_EXEC | VM_READ;
    insert_vm_struct(mm, vma);
    
    vma = vm_area_alloc(mm);
    vma->vm_end = PAGE_ALIGN(STACK_TOP_MAX);
    vma->vm_start = (vma->vm_end - PGSIZE) & PAGE_MASK;
    vma->vm_flags = VM_STACK_FLAGS;
    current->mm->stack_vm = current->mm->total_vm = 1;
    mm->brk = mm->start_brk = USER_MEM_START;
    insert_vm_struct(mm, vma);
    safestrcpy(current->name, "initcode", strlen("initcode") + 1);
    current->cwd = namei("/");
    return 0;
}

int do_fork(void)
{
    int pid = copy_process(TASK_NORMAL, 0, 0, current->name);
    /*父进程返回值为子进程的pid*/
    return pid;
}
