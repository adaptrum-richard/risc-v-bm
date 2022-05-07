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
#include "virtio_disk.h"
#include "fs.h"
#include "bio.h"

void delay()
{
    int j = 0;
    for(int i ; i < 100000; i++){
        j = i % 3;
        j++;
    }
}

void kernel_process(uint64 arg)
{
    while(1){
        sleep(arg);
        printk("current %s run\n", current->name);
    }
}

void idle()
{
    //idle可以用来做负载均衡
    binit();
    fsinit(ROOTINO);
    while(1){
        sleep(2);
        printk("current %s run\n", current->name);
    }
}

void run_proc()
{
    int ret = copy_process(PF_KTHREAD, (uint64)&idle, 0, "idle");
    if(ret < 0)
        panic("copy_process error ,arg = 0\n");

    ret = copy_process(PF_KTHREAD, (uint64)&kernel_process, 1, "kernel_process1");
    if(ret < 0)
        panic("copy_process error ,arg = 1\n");

    ret = copy_process(PF_KTHREAD, (uint64)&kernel_process, 2, "kernel_process2");
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
        virtio_disk_init();
        run_proc();
        intr_on();
        for(;;){
            schedule();
        }
    }else{
        while(1);
    }
}
