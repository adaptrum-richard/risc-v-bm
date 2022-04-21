#include "riscv.h"
#include "printk.h"
#include "console.h"
#include "proc.h"
#include "kalloc.h"

void main()
{
    if(cpuid() == 0)
    {
        consoleinit();
        printfinit();
        printk("hello world\n");
        kinit();
        while(1);
    }else{
        while(1);
    }
}
