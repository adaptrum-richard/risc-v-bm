#include "riscv.h"
#include "printk.h"
#include "console.h"
#include "proc.h"

void main()
{
    if(cpuid() == 0)
    {
        consoleinit();
        printfinit();
        printk("hello world\n");
        while(1);
    }else{
        while(1);
    }
}