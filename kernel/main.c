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
#include "page.h"
#include "exec.h"
#include "pci.h"
volatile int init_done_flag = 0;
static volatile int fs_init_done = 0;
void kernel_process(uint64 arg)
{
    while(1){
        kernel_sleep(1);
        printk("hart%d current %s run pid:%d\n", smp_processor_id(),current->name, current->pid);
        //delay();
    }
}

void idle()
{
    //文件系统初始化
    if(smp_processor_id() == 0){
        binit();
        iinit();
        fsinit(ROOTINO);//这里需要中断
        fileinit();
        fs_init_done = 1;
        __smp_wmb();
    }else 
        while(fs_init_done == 1)
            __smp_rmb();
    while(1){
        printk("current %s run pid:%d, cpu%d, init_done_flag = %d\n", current->name, current->pid,
            smp_processor_id(), init_done_flag);
        kernel_sleep(1);
        //traversing_rq();
    }
}


extern void set_pgd(uint64);

void copy_to_user_thread(uint64 arg)
{
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

    printk("user_beigin = 0x%lx, user_end = 0x%lx, pc = 0x%lx, process = 0x%lx\n",
        begin, end, proccess - begin, proccess);
    acquire(&current->lock);
    int err = move_to_user_mode(begin, end - begin, proccess - begin);
    release(&current->lock);

    free_page(begin);

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
    char name[8] = {0};
    sprintf(name, "idle%d", smp_processor_id());
    //printk("name = %s\n", name);
    ret = copy_process(PF_KTHREAD, (uint64)&idle, 0, name);
    if(ret < 0)
        panic("copy_process error ,arg = 0\n");
    sprintf(name, "process%d", smp_processor_id());
    ret = copy_process(PF_KTHREAD, (uint64)&kernel_process, 1, name);
    if(ret < 0)
        panic("copy_process error ,arg = 1\n");
#if 0
    ret = copy_process(PF_KTHREAD, (uint64)&kernel_process, 2, "kernel_process2");
    if(ret < 0)
        panic("copy_process error ,arg = 2\n");
#endif
#if TEST_FILE
    ret = copy_process(PF_KTHREAD, (uint64)&test_sysfile, (uint64)"aaa", "test_sysfile");
    if(ret < 0)
        panic("copy_process error ,arg = 2\n");
#endif
    if(smp_processor_id() == 0){
        ret = copy_process(PF_KTHREAD, (uint64)&copy_to_user_thread, 0, "kernel_init");
        if(ret < 0)
            panic("copy_process error init\n");
    }
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
        set_init_task_to_current();
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
        init_sleep_queue();
        sched_init();
        init_process();
        __sync_synchronize();
        virtio_disk_init();
        net_init();
        pci_init();
        run_proc();
        intr_on();
        __sync_synchronize();
        init_done_flag = 1;
        __sync_synchronize();
        printk("init_done_flag = %d\n", init_done_flag);
        //while(1){
            //printk("current %s run pid:%d\n", current->name, current->pid);
        //    schedule();
        //    
        //}
    }else{

        while(init_done_flag == 0)
            __sync_synchronize();
        init_done_flag = smp_processor_id();
        sched_init();
        init_done_flag = 5;
        init_process();
        init_done_flag = 6;
        printk("hart%d run\n", smp_processor_id());
        w_sscratch(0);
        kvminithart();
        trapinithart();
        //plicinithart(); /*外设中断在cpu0上处理，其他CPU不用初始化*/
        printk("preemt count = %d\n", current->preempt_count);
        run_proc();
    }
    traversing_rq();
    while(1){
        schedule();
        free_zombie_task();
    }

}
