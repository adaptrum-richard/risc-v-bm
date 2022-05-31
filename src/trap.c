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
        printk("status = 0x%lx, cause = 0x%lx, badaddr = 0x%lx\n", 
            regs->status, regs->cause, regs->badaddr);
        /*sstatus = 0x100 stval = 0x8
        sstatus 第8位是1，表示是从s-mode发生了缺页，说明有问题
        */
        panic("load page fault exception\n");
        break;
    case EXC_STORE_PAGE_FAULT:
        printk("status = 0x%lx, cause = 0x%lx, badaddr = 0x%lx\n", 
            regs->status, regs->cause, regs->badaddr);
        panic("store page fault exception\n");
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
