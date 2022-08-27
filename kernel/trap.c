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
#include "debug.h"
#include "e1000.h"
#include "stacktrace.h"
#include "types.h"

void kernelvec();
extern void print_symbol(unsigned long addr);
// set up to take exceptions and traps while in the kernel.
void trapinithart(void)
{
    w_stvec((uint64)kernelvec);
}

void devintr()
{
    /*supervisor mode extension interrupt*/
    int irq = plic_claim();
    if(irq == UART0_IRQ)
        uartintr();
    else if (irq == VIRTIO0_IRQ)
        virtio_disk_intr();
    else if (irq == E1000_IRQ)
        e1000_intr();
    else
        printk("unexpected interrupt irq = %d\n", irq);

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
    printk_intr("%s %d: addr = 0x%lx,pid = %d, name = %s\n",
         __func__, __LINE__, addr, current->pid, current->name);
    if (user_mode(regs)) {
        /*TODO: 不用panic，杀死进程*/
        do_trap(regs, SIGSEGV, code, addr);
        return;
    }

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
    pr_debug("mmap: va %lx, pa %lx\n", address & PAGE_MASK, mem);  
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
        printk("addr = 0x%lx\n", addr);
        panic("access to user memory without uaccess routines");
    }

    if (cause == EXC_STORE_PAGE_FAULT)
        flags |= FAULT_FLAG_WRITE;
    else if (cause == EXC_INST_PAGE_FAULT){
        flags |= FAULT_FLAG_INSTRUCTION;
    }

    vma = find_vma(mm, addr);
    if(!vma){
        pr_err("don't find vma\n");
        bad_area(regs, mm, code, addr);
        return;
    }
    if (vma->vm_start <= addr)
        goto good_area;

    if (!(vma->vm_flags & VM_GROWSDOWN)) {
        pr_err("don't grow stack down: vm_start: %lx, vm_end:%lx\n", 
            vma->vm_start, vma->vm_end);
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
        printk("cpu status register:0x%lx, badaddr:0x%lx\n", regs->status, regs->badaddr);
        printk("task pid:%d, task name: %s, parent pid = %d\n", current->pid, current->name, current->parent ? current->parent->pid : -1);
        printk("current pagetable:%lx\n", current->mm->pagetable);
        print_all_vma(current->mm->pagetable, current->mm->mmap);
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

void show_regs(struct pt_regs *regs)
{
	printk("task: %p\n", current);
	printk("PC is at 0x%016llx\n", regs->epc);
	printk("LR is at 0x%016llx\n", regs->ra);
    printk("epc:  %016llx ra: %016llx sp:  %016llx\n", regs->epc, regs->ra, regs->sp);
    printk(" gp:  %016llx tp: %016llx t0:  %016llx\n", regs->gp, regs->tp, regs->t0);
    printk(" t1:  %016llx t2: %016llx s0:  %016llx\n", regs->t1, regs->t2, regs->s0);
    printk(" s1:  %016llx a0: %016llx a1:  %016llx\n", regs->s1, regs->a0, regs->a1);
    printk(" a2:  %016llx a3: %016llx a4:  %016llx\n", regs->a2, regs->a3, regs->a4);
    printk(" a5:  %016llx a6: %016llx a7:  %016llx\n", regs->a5, regs->a6, regs->a7);
    printk(" s2:  %016llx s3: %016llx s4:  %016llx\n", regs->s2, regs->s3, regs->s4);
    printk(" s5:  %016llx s6: %016llx s7:  %016llx\n", regs->s5, regs->s6, regs->s7);
    printk(" s8:  %016llx s9: %016llx s10: %016llx\n", regs->s8, regs->s9, regs->s10);
    printk(" s11: %016llx t3: %016llx t4:  %016llx\n", regs->s11, regs->t3, regs->t4);
    printk(" t5:  %016llx t6: %016llx\n", regs->t5, regs->t6);
    printk("status: %016llx badaddr: %016llx cause:  %016llx\n", regs->status, regs->badaddr, regs->cause);
}

static int unwind_frame(struct stackframe *frame)
{
	unsigned long high, low;
	unsigned long fp = frame->fp;
    
    low = frame->sp;
    high = ALIGN(low, THREAD_SIZE);

    /* fp在栈顶部和栈底之间*/
	if (fp < low || fp > high)
		return -EINVAL;
    /* fp必须16字节对齐*/
	if (fp & 0xf)
		return -EINVAL;
    
    /*子函数栈的FP存储了父函数FP的值*/
	frame->fp = *(unsigned long *)(fp - 16);
	frame->sp = fp;
    /*
	 * 根据子函数栈帧里保存的LR可以间接
	 * 获取父函数调用子函数时的PC值
	 *
	 * 在调用子函数时，LR指向子函数
	 * 返回的下一条指令
	 *
	 * PC_f=*LR_c-4=*(FP_c-8)-4
	 *
	 * PC_f: 指的是父函数调用子函数时
	 * 的PC值
	 */
    //参考：https://blog.csdn.net/dai_xiangjun/article/details/126541174
	frame->pc = *(unsigned long *)(fp-8) - 4;
    return 0;
}

void dump_backtrace(struct pt_regs *regs, struct task_struct *p)
{
	struct stackframe frame;

	if (regs) {
		show_regs(regs);

		frame.fp = regs->s0;
		frame.sp = regs->sp;
		frame.pc = regs->epc;
	} else if (p == current) {
		frame.fp = (unsigned long)__builtin_frame_address(0);
		frame.sp = current_stack_pointer;
		frame.pc = (unsigned long)dump_backtrace;
	}

	printk("Call trace:\n");
	while (1) {
		unsigned long where = frame.pc;
		int ret;

		ret = unwind_frame(&frame);
		if (ret < 0)
			break;

		print_symbol(where);
	}
}
