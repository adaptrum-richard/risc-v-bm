#include "riscv.h"
#include "printk.h"
#include "console.h"
#include "proc.h"
#include "mm.h"
#include "vm.h"
#include "trap.h"
#include "plic.h"
#include "fork.h"
#include "sched.h"
#include "virtio_disk.h"
#include "fs.h"
#include "bio.h"
#include "sysfile.h"
#include "fcntl.h"
#include "string.h"
#include "file.h"
#include "sleep.h"
#include "preempt.h"
#include "memlayout.h"
#include "testcode.h"
#include "page.h"
#include "exec.h"

static volatile int fs_init_done = 0;
void kernel_process(uint64 arg)
{
    while(1){
        kernel_sleep(1);
        printk("1 current %s run pid:%d\n", current->name, current->pid);
        //delay();
    }
}

void idle()
{
    //文件系统初始化
    binit();
    iinit();
    fsinit(ROOTINO);//这里需要中断
    fileinit();
    fs_init_done = 1;
    __smp_wmb();
    while(1){
        
        printk("current %s run pid:%d\n", current->name, current->pid);
        kernel_sleep(1);
        //traversing_rq();
    }
}

#ifndef TEST_READ_INIT_CODE_AND_RUN
extern char user_begin[], user_end[], user_process[];
#endif
extern void set_pgd(uint64);

void copy_to_user_thread(uint64 arg)
{
#ifdef TEST_READ_INIT_CODE_AND_RUN
    unsigned long pc = -1;
    unsigned long size = -1;
    unsigned long end;
    unsigned long proccess;
    unsigned long begin = get_free_page();
    while(fs_init_done == 0);
    int ret = read_initcode((unsigned long*)begin, &size, &pc);
    if(ret < 0){
        panic("copy_to_user_thread");
    }
    proccess = pc + begin - USER_CODE_VM_START;
    end = begin + size;
#else
    unsigned long begin = (unsigned long)&user_begin;
    unsigned long end = (unsigned long)&user_end;
    unsigned long proccess = (unsigned long)&user_process;
#endif
    printk("user_beigin = 0x%lx, user_end = 0x%lx, pc = 0x%lx, process = 0x%lx\n",
        begin, end, proccess - begin, proccess);
    acquire(&current->lock);
    int err = move_to_user_mode(begin, end - begin, proccess - begin);
    release(&current->lock);
#ifdef TEST_READ_INIT_CODE_AND_RUN
    free_page(begin);
#endif
    if(err < 0)
        panic("move_to_user_mode\n");
    preempt_disable(); //切换到第一个用户进程的时候，不能被抢占
    intr_off();//先关闭中断，在恢复pt_regs时会开启中断
    /*此函数返回时，就切换到用户空间，所以这里需要设置sscratch，
    在异常向量表中，通过sscratch判断是来自用户空间还是内核空间*/
    w_sscratch((uint64)current); 
    set_pgd(MAKE_SATP(current->mm->pagetable)); /*立即切换页表*/
    preempt_enable();
}

void run_proc()
{
    int ret ;
    ret = copy_process(PF_KTHREAD, (uint64)&idle, 0, "idle");
    if(ret < 0)
        panic("copy_process error ,arg = 0\n");
#if 0
    ret = copy_process(PF_KTHREAD, (uint64)&kernel_process, 1, "kernel_process1");
    if(ret < 0)
        panic("copy_process error ,arg = 1\n");

    ret = copy_process(PF_KTHREAD, (uint64)&kernel_process, 2, "kernel_process2");
    if(ret < 0)
        panic("copy_process error ,arg = 2\n");
#endif
#if TEST_FILE
    ret = copy_process(PF_KTHREAD, (uint64)&test_sysfile, (uint64)"aaa", "test_sysfile");
    if(ret < 0)
        panic("copy_process error ,arg = 2\n");
#endif
    ret = copy_process(PF_KTHREAD, (uint64)&copy_to_user_thread, 0, "kernel_init");
    if(ret < 0)
        panic("copy_process error init\n");

}

void bge_test()
{
    unsigned long u = 0x100;
    unsigned long ret = 0;
    unsigned long tmp = 0;
    asm volatile(
        "bge %1,%2, 1f \n"
        "addi %0, %0, 3\n"
        "1:\n" 
        "addi %0, %0, 1\n"
        :"=r"(ret)
        :"r"(u), "r"(tmp)
        :"memory"
    );
    printk("========================ret = 0x%lx\n", ret);
}

void main()
{
    if(smp_processor_id() == 0)
    {
        w_sscratch(0);
        consoleinit();
        printkinit();
        printk("boot\n");
        mem_init();
        kvminit();
        kvminithart();
        trapinithart();
        plicinit();
        plicinithart();
        sched_init();
        init_process();
        __sync_synchronize();
        virtio_disk_init();
        run_proc();
        intr_on();
        while(1){
            //printk("current %s run pid:%d\n", current->name, current->pid);
            schedule();
            free_zombie_task();
        }
    }else{
        while(1);
    }
}
