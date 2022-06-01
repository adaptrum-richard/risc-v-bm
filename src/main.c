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
#define TEST_FILE 1
static volatile int init_done = 0;
static void delay()
{
    int j = 0;
    for(int i ; i < 100000000; i++){
        j = i % 3;
        j++;
    }
}

void test_console()
{
    int fd, ret;
    char buf[8];
    if((fd = __sys_open("/console", O_RDWR)) < 0){
        __sys_mknod("/console", CONSOLE, 0);
        fd = __sys_open("/console", O_RDWR);
    }
    if(fd <  0){
        panic("test_console\n");
    }
    __sys_close(fd);
    int max = 100;
    while(1){
        fd = __sys_open("/console", O_RDWR);
        if(fd <  0){
            panic("test_console\n");
        }   
        memset(buf, 0x0, sizeof(buf));
        int i = 0;
        while(i < max){
            char c;
            ret = __sys_read(fd, &c, 1);
            if(ret < 1)
                break;
            buf[i++] = c;
            if(c == '\n' || c == '\r')
                break;
        }
        buf[i] = '\0';
        int j = 0;
        while(buf[j] != '\0'){
            __sys_write(fd, &buf[j], 1);
            j++;
        }
        __sys_close(fd);
    }
 
    __sys_close(fd);
}

void test_sysfile(uint64 arg)
{
    int ret;
    char path[MAXPATH] = {0};
    char buf[MAXPATH] = {0};
    while(init_done == 0);

    sprintf(path, "/tmp/%s", (char*)arg);
    int fd = __sys_open("/test", O_CREATE| O_WRONLY);

    if(fd < 0){
        printk("%s:%d __sys_open %d failed\n", __func__, __LINE__, fd);
        panic("open failed\n");
    }
    for(int i = 0; i < 3; ++i){
        ret = __sys_write(fd, path, strlen(path));
        if(ret < 0){
            printk("%s:%d __sys_write failed\n", __func__, __LINE__);
            panic("failed");
        }
        printk("%s:%d: write  %s , len = %d\n", __func__, __LINE__, path, ret);
        //sleep(1);
    }
    __sys_close(fd);

    fd = __sys_open("/test", O_RDONLY);
    if(fd < 0){
        printk("%s:%d __sys_open %d failed\n", __func__, __LINE__, fd);
        panic("open failed\n");
    }
    
    ret = __sys_read(fd, buf, sizeof(buf));
    __sys_close(fd);
    printk("read %s, len = %d\n", buf, ret);
    test_console();
}

void kernel_process(uint64 arg)
{
    while(1){
        sleep(1);
        printk("1 current %s run pid:%d\n", current->name, current->pid);
        //delay();
    }
}

void idle()
{
#if TEST_FILE
    //idle可以用来做负载均衡
    binit();
    fsinit(ROOTINO);
    fileinit();
 #endif  
    init_done = 1;
    while(1){
        
        printk("current %s run pid:%d\n", current->name, current->pid);
        sleep(1);
        //traversing_rq();
    }
}
extern char user_begin[], user_end[], user_process[];
extern void set_pgd(uint64);
void copy_to_user_thread(uint64 arg)
{
    unsigned long begin = (unsigned long)&user_begin;
    unsigned long end = (unsigned long)&user_end;
    unsigned long proccess = (unsigned long)&user_process;
    printk("user_beigin = 0x%lx, user_end = 0x%lx, pc = 0x%lx, process = 0x%lx\n",
        begin, end, proccess - begin, proccess);
    acquire(&current->lock);
    int err = move_to_user_mode(begin, end - begin, proccess - begin);
    release(&current->lock);
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
    ret = copy_process(PF_KTHREAD, (uint64)&copy_to_user_thread, 0, "init");
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
            //sleep(3);
            //printk("current %s run pid:%d\n", current->name, current->pid);
            delay();
        }
    }else{
        while(1);
    }
}
