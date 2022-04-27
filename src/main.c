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

void kernel_process1(uint64 arg)
{
    uchar buff[1024] = {0};
    while(1){
        sleep(6);
        disk_read_block(buff, 10);
        printk("read block value is \"%s\"\n", buff);
    }
}

void kernel_process2(uint64 arg)
{
    uchar buff[1024] = {0};
    int i = 0;
    while(1){
        sleep(3);
        sprintf((char *)buff, "string = %d", i++);
        //printk("arg = %d\n", (int)arg);
        disk_write_block(buff, 10);
    }
}

void idle()
{
    //idle可以用来做负载均衡
    while(1);
}

void run_proc()
{
    int ret = copy_process(PF_KTHREAD, (uint64)&idle, 0);
    if(ret < 0)
        panic("copy_process error ,arg = 0\n");

    ret = copy_process(PF_KTHREAD, (uint64)&kernel_process1, 1);
    if(ret < 0)
        panic("copy_process error ,arg = 1\n");

    ret = copy_process(PF_KTHREAD, (uint64)&kernel_process2, 2);
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
