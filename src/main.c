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
#include "sysfile.h"
#include "fcntl.h"
#include "string.h"
#include "file.h"

static volatile int init_done = 0;
void delay()
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
        //sleep(arg);
        //printk("current %s run\n", current->name);
        delay();
    }
}

void idle()
{
    //idle可以用来做负载均衡
    binit();
    fsinit(ROOTINO);
    fileinit();
    init_done = 1;
    while(1){
        //sleep(2);
        //printk("current %s run\n", current->name);
        delay();
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
    ret = copy_process(PF_KTHREAD, (uint64)&test_sysfile, (uint64)"aaa", "test_sysfile");
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
            //sleep(10);
            //delay();
            schedule();
        }
    }else{
        while(1);
    }
}
