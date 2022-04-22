#include "riscv.h"
#include "printk.h"
#include "console.h"
#include "proc.h"
#include "kalloc.h"
#include "vm.h"
#include "trap.h"
#include "plic.h"
#include "fork.h"
#include "sched.h"

void kernel_process(uint64 arg)
{
    while(1){
        for(int i = 0 ; i < 0xfffffff; i++)
        {
            if(i == 0xffff)
                printk("arg = %d\n", (int)arg);
        }
    }
}

void run_proc()
{
    int ret = copy_process(PF_KTHREAD, (uint64)&kernel_process, 1);
    if(ret < 0)
        panic("copy_process error ,arg = 1\n");

    ret = copy_process(PF_KTHREAD, (uint64)&kernel_process, 2);
    if(ret < 0)
        panic("copy_process error ,arg = 2\n");
}

void main()
{
    if(cpuid() == 0)
    {
        consoleinit();
        printkinit();
        printk("boot\n");
        kinit();
        kvminit();
        kvminithart();
        trapinithart();
        plicinit();
        plicinithart();
        __sync_synchronize();
        run_proc();
        intr_on();
        for(;;){
            schedule();
        }
    }else{
        while(1);
    }
}
