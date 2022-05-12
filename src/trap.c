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

int devintr()
{
    uint64 scause = r_scause();

    if( (scause & (1L<<63)) && (scause & 0xff) == 0x9 ){
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
        return 1;
    } else if ( scause == ( (1L << 63) | 0x1L)) {
        //printk("cpu id = %d, rcv timer interrupt\n", cpuid());
        w_sip(r_sip() & ~(SSTATUS_SIE));
        timer_tick();
        return 2;
    } else
        return 0;

}

void kerneltrap()
{
    int which_dev = 0;
    uint64 sepc = r_sepc();
    uint64 sstatus = r_sstatus();
    uint64 scause = r_scause();
    if(scause & ( 1UL<< 63))
        irq_enter();
    
    if((sstatus & SSTATUS_SPP) == 0){
        panic("kerneltrap: not from supervisor mode");
    }
    if(intr_get() != 0){
        panic("kerneltrap: interrupts enabled");
    }

    if((which_dev = devintr()) == 0){
        printk("scause %p\n", scause);
        printk("sepc=%p stval=%p\n", r_sepc(), r_stval());
        panic("kerneltrap");
    }


    if(scause & ( 1UL<< 63))
        irq_exit();
    /*maybe schedule thread*/

    /*restore programer counter and supervisor mode status*/
    w_sepc(sepc);
    w_sstatus(sstatus);
}
