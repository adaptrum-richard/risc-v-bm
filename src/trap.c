#include "printk.h"
#include "riscv.h"
#include "memlayout.h"
#include "plic.h"
#include "proc.h"
#include "sched.h"
#include "virtio_disk.h"
#include "hardirq.h"
#include "uart.h"
#include "pt_regs.h"
#include "vm.h"
#include "mm.h"
#include "signal.h"
#include "siginfo.h"
#include "mmap.h"
#include "errorno.h"
#include "string.h"
#include "vm.h"

void kernelvec();

// set up to take exceptions and traps while in the kernel.
void trapinithart(void)
{
    w_stvec((uint64)kernelvec);
}

void devintr()
{
    /*supervisor mode extension interrupt*/
    int irq = plic_claim();
    if(irq == UART0_IRQ){
        uartintr();
    } else if (irq == VIRTIO0_IRQ){
        virtio_disk_intr();
        //printk("rcv virtio0 irq\n");
    } else {
        printk("unexpected interrupt irq = %d\n", irq);
    }

    if(irq)
        plic_complete(irq);
}

void do_trap(struct pt_regs *regs, int signo, int code, unsigned long addr)
{
    /*TODO*/
    printk_intr("%s %d\n", __func__, __LINE__);
    panic("do_trap");
}

static inline void bad_area(struct pt_regs *regs, 
    struct mm_struct *mm, int code, unsigned long addr)
{
    if (user_mode(regs)) {
        /*TODO: 不用panic，杀死进程*/
        do_trap(regs, SIGSEGV, code, addr);
        return;
    }
    printk_intr("%s %d\n", __func__, __LINE__);
    panic("bad_area");
}

vm_fault_t handle_mm_fault(struct vm_area_struct *vma, unsigned long address,
             unsigned int flags, struct pt_regs *regs)
{
    uint64 mem;
    //uint64 va_start = vma->vm_start;
    int perm = 0;
    if(flags & FAULT_FLAG_USER)
        perm |= PTE_U;
    if(vma->vm_flags & VM_EXEC)
        perm |= PTE_X;
    if(vma->vm_flags & VM_READ)
        perm |= PTE_R;
    if(vma->vm_flags & VM_WRITE)
        perm |= PTE_W;

    mem = (uint64)get_free_page();
    if(!mem){
        return -ENOMEM;
    }
    memset((void *)mem, 0, PGSIZE);
    mappages(current->mm->pagetable, address & PAGE_MASK, PGSIZE, mem, perm);    
    return 0;
}

void do_page_fault(struct pt_regs *regs)
{
    struct vm_area_struct *vma;
    unsigned int flags = FAULT_FLAG_DEFAULT;
    struct mm_struct *mm;
    struct task_struct *tsk;
    unsigned long addr, cause;
    int code = SEGV_MAPERR;

    tsk = current;
    mm = tsk->mm;
    addr = regs->badaddr;
    cause = regs->cause;
    
    if(regs->status & SSTATUS_SPIE)
        intr_on();
    
    if(in_atomic() || !mm){
        /*do page fault不能能发送在in_atomic或没有mm的进程中*/
        panic("do_page_fault\n");
    }

    if (user_mode(regs))
        flags |= FAULT_FLAG_USER;

    if(!user_mode(regs) && addr < USER_MEM_START 
            && !(regs->status & SSTATUS_SUM)){
        panic("access to user memory without uaccess routines");
    }

    if (cause == EXC_STORE_PAGE_FAULT)
        flags |= FAULT_FLAG_WRITE;
    else if (cause == EXC_INST_PAGE_FAULT){
        flags |= FAULT_FLAG_INSTRUCTION;
    }

    vma = find_vma(mm, addr);
    if(!vma){
        bad_area(regs, mm, code, addr);
        return;
    }
    if (vma->vm_start <= addr)
        goto good_area;

    if (!(vma->vm_flags & VM_GROWSDOWN)) {
        bad_area(regs, mm, code, addr);
        return;
    }

    if (expand_stack(vma, addr)) {
        bad_area(regs, mm, code, addr);
        return;
    }

good_area:
    code = SEGV_ACCERR;
    handle_mm_fault(vma, addr, flags, regs);
}

void hanlder_exception(struct pt_regs *regs)
{
    /*异常处理*/
    uint64 exception_code = (regs->cause &(~(1UL<<63)));
    switch (exception_code)
    {
    case EXC_SYSCALL:
        panic("syscall exception\n");
        break;
    case EXC_LOAD_PAGE_FAULT:
    case EXC_STORE_PAGE_FAULT:
        do_page_fault(regs);
        break;
    default:
        printk("don't support exception, code:%lu\n", exception_code);
        printk("scause 0x%x\n", regs->cause);
        printk("status = 0x%x badaddr= 0x%x\n", regs->status, regs->badaddr);
        panic("kerneltrap");
        break;
    }
}

void handler_irq(struct pt_regs *regs)
{
    uint64 exception_code = (regs->cause &(~(1UL<<63)));
    irq_enter();
    
    if(intr_get() != 0){
        panic("kerneltrap: interrupts enabled");
    }
   
    switch (exception_code) {
    case IRQ_S_EXT:
        devintr();
        break;
    case IRQ_S_SOFT:
        /*s模式的软件中断来时m模式的时钟中断*/
        w_sip(r_sip() & ~(SSTATUS_SIE));
        timer_tick();
        break;
    default:
        printk("don't support interrupt\n");
        printk("scause %p\n", regs->cause);
        printk("epc=%p badaddr=%p\n", regs->epc, regs->badaddr);
        panic("kerneltrap");
        break;
    }

    irq_exit();
}
