#include "printk.h"
#include "riscv.h"
#include "memlayout.h"
#include "plic.h"
#include "proc.h"
#include "sched.h"
#include "virtio_disk.h"
#include "hardirq.h"
#include "uart.h"

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

void kerneltrap()
{
    uint64 sepc = r_sepc();
    uint64 sstatus = r_sstatus();
    uint64 scause = r_scause();
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
            printk("scause %p\n", scause);
            printk("sepc=%p stval=%p\n", r_sepc(), r_stval());
            panic("kerneltrap");
            break;
        }
    }else {
        /*异常处理*/
        switch (exception_code)
        {
        case EXC_SYSCALL:
            break;
        case EXC_LOAD_ACCESS:
            break;
        case EXC_STORE_ACCESS:
            break;
        default:
            printk("scause %p\n", scause);
            printk("sepc=%p stval=%p\n", r_sepc(), r_stval());
            panic("kerneltrap");
            break;
        }
    }

    if(intr_flag)
        irq_exit();
    /*maybe schedule thread*/

    /*restore programer counter and supervisor mode status*/
    w_sepc(sepc);
    w_sstatus(sstatus);
}
