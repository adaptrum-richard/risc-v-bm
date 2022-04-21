#include "printk.h"
#include "riscv.h"
void kernelvec();

// set up to take exceptions and traps while in the kernel.
void trapinithart(void)
{
  w_stvec((uint64)kernelvec);
}

void kerneltrap()
{
    printk("kerneltrap\n");
}

