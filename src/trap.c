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

void kerneltrap(struct pt_regs *regs)
{
    uint64 sepc = regs->epc;
    uint64 sstatus = regs->status;
    uint64 scause = regs->cause;
    uint64 intr_flag = scause & ( 1UL<< 63);
    uint64 exception_code = (scause &(~(1UL<<63)));
    if(intr_flag)
        irq_enter();
    
    if((sstatus & SSTATUS_SPP) == 0){
        panic("kerneltrap: not from supervisor mode");
    }
    
    if(intr_get() != 0){
        panic("kerneltrap: interrupts enabled");
    }

    if(intr_flag){
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
            printk("scause %p\n", scause);
            printk("sepc=%p stval=%p\n", regs->epc, regs->badaddr);
            panic("kerneltrap");
            break;
        }
    }else {
        /*异常处理*/
        switch (exception_code)
        {
        case EXC_SYSCALL:
            panic("syscall exception\n");
            break;
        case EXC_LOAD_PAGE_FAULT:
            printk("sstatus = 0x%lx, spec = 0x%lx, stval = 0x%lx\n", 
                sstatus, sepc, r_stval());
            /*sstatus = 0x100 stval = 0x8
            sstatus 第8位是1，表示是从s-mode发生了缺页，说明有问题
            */
            panic("load page fault exception\n");
            break;
        case EXC_STORE_PAGE_FAULT:
            printk("sstatus = 0x%lx, spec = 0x%lx, stval = 0x%lx\n", 
                sstatus, sepc, r_stval());
            panic("store page fault exception\n");
            break;
        default:
            printk("don't support exception, code:%lu\n", exception_code);
            printk("scause %p\n", scause);
            printk("sepc=%p stval=%p\n", r_sepc(), r_stval());
            panic("kerneltrap");
            break;
        }
    }

    /*判断返回用户空间还是内核空间*/
    if(!(sstatus & SSTATUS_SPP)){
        /*返回用户空间时，将task写入到sscratch*/
        panic("not\n");
        w_sscratch((uint64)current);
        w_tp((uint64)current);
    }
    
    if(intr_flag)
        irq_exit();
    
    /*restore programer counter and supervisor mode status*/
    //w_sstatus(sstatus);
    //w_sepc(sepc);
}
