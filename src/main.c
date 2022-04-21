#include "riscv.h"
#include "printk.h"
#include "console.h"
#include "proc.h"
#include "kalloc.h"
#include "vm.h"
#include "trap.h"
#include "plic.h"

void main()
{
    if(cpuid() == 0)
    {
        consoleinit();
        printfinit();
        printk("boot\n");
        kinit();
        kvminit();
        kvminithart();
        printk("hello world\n");
        trapinithart();
        plicinit();
        plicinithart();
        __sync_synchronize();
        intr_on();
        while(1);
    }else{
        while(1);
    }
}
