#include "elf.h"
#include "riscv.h"
#include "types.h"
#include "memlayout.h"
#include "printk.h"
#include "fs.h"
#include "vm.h"
#include "proc.h"
#include "log.h"
#include "page.h"

/*
prog_start: program的开始地址，
prog_size: 运行程序的大小
pc: 运行程序的开始偏移值
initcode.out代码只有1KB左右,此函数只能读取program大小小于4KB的程序。
*/
int read_initcode(uint64 *prog_start, uint64 *prog_size, uint64 *pc)
{

    struct inode *ip;
    struct elfhdr elf;
    struct proghdr ph;
    char *path = "/initcode.out";
    if (!path)
        return -1;

    log_begin_op();
    if ((ip = namei(path)) == 0){
        log_end_op();
        return -1;
    }
    ilock(ip);
    if (readi(ip, (uint64)&elf, 0, sizeof(elf)) != sizeof(elf))
        goto bad;
    
    if (elf.magic != ELF_MAGIC)
        goto bad;

    // Load program into memory.
    if (readi(ip, (uint64)&ph, elf.phoff, sizeof(ph)) != sizeof(ph)){
        printk("%s %d: read program header failed\n", __func__, __LINE__);
        goto bad;
    }

    if (ph.type != ELF_PROG_LOAD){
        printk("%s %d: read program header type error\n", __func__, __LINE__);
        goto bad;
    }

    if (ph.memsz < ph.filesz){
        printk("%s %d: read program header, memsz: 0x%llx, filesz:0x%llx\n", 
            __func__, __LINE__, ph.memsz, ph.filesz);
        goto bad;
    }

    if (ph.vaddr + ph.memsz < ph.vaddr){
        printk("%s %d: read program header, memsz: 0x%llx, vaddr:0x%llx\n", 
            __func__, __LINE__, ph.memsz, ph.vaddr);
        goto bad;
    }

    if(readi(ip, (uint64)prog_start, ph.off, ph.filesz) != ph.filesz){
        printk("%s %d: read program header, memsz: 0x%llx, vaddr:0x%llx\n", 
            __func__, __LINE__, ph.memsz, ph.vaddr);
        goto bad;
    }
    iunlockput(ip);
    log_end_op();
    *prog_size = ph.filesz;
    *pc = elf.entry;
    return 0;
bad:
    if(ip){
        iunlock(ip);
        log_end_op();
    }
    return -1;
}

/*
linux内核中exec函数实现的流程：
exec系列函数从linux的linux_binfmt链表中，通过依次调用每个结构的load_binary函数来选择合适的运行格式，
一旦找到就执行load_binary函数，否则尝试下一个linux_binfmt的load_binary，直到尝试完所有的linux_binfmt。
load_binary函数：
1.检查128位的magic number，看文件是不是属于这个格式
2.读取文件的header
3.从文件得到dynamic linker的位置
4.检查dynamic linker是否有效
5.调用flush_old_exec()函数，清除被之前计算所使用的所有资源，像内存，页表
6.使用do_mmap()将可执行文件的text，data，bss段映射到进程中
7.如果可执行文件还有其它段，也映射到进程中
8.加载dynamic linker
9.由dynamic linker将程序运行所需要的其它库用mmap()映射到进程中
10.跳到程序的入口出开始执行程序

问题：
1.在加载新程序的代码段到老进程的代码段后，切换页表后，为什么不会出现问题？
*/
int exec(char *path, char *argv)
{
    return 0;
}
