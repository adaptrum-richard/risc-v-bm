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
#include "mmap.h"

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
不会出问题，原因是在执行exec函数时，处于内核空间，没有执行应用程序的代码，
可以释放掉用户旧的代码，替换成新的代码
2.
*/
int exec(char *path, char *argv)
{

    char *s, *last;
    int i, off, j;
    uint64 argc, sz = 0;
    struct elfhdr elf;
    struct proghdr ph;
    struct inode *ip;
    pagetable_t pagetable = 0, oldpagetable;
    struct vm_area_struct *vma = NULL;
    struct task_struct *p = current;
    uint64 vm_addr, vm_size;
    uint64 pa, sp, stackbase, stack[MAXARG];
    uint64 program_size = 0;

    log_begin_op();
    if((ip = namei(path)) == 0){
        log_end_op();
        printk("%s %d: namei path '%s' failed\n", __func__, __LINE__, path);
        return -1;
    }

    ilock(ip);
    if(readi(ip, (uint64)&elf, 0, sizeof(elf))!= sizeof(elf)){
        printk("%s %d: readi elf failed\n", __func__, __LINE__);
        goto bad;
    }
    /*复制一份内核的页表*/
    pagetable = copy_kernel_tbl();
    if(!pagetable){
        printk("%s %d: copy_kernel_tbl failed\n", __func__, __LINE__);
        goto bad;
    }

    /*第一个vma是用来存放代码段的*/
    vma = p->mm->mmap;
    vm_addr = vma->vm_start;
    //vm_size = PAGE_ALIGN(vma->vm_end - vma->vm_start);
    //Load program into memory.

    pa = get_free_page();
    for(i = 0, off = elf.phoff; i < elf.phnum; i++, off += sizeof(ph)){
        
        if(readi(ip, 0, (uint64)&ph, off, sizeof(ph)) != sizeof(ph))
            goto bad;
        if(ph.type != ELF_PROG_LOAD)
            continue;
        if(ph.memsz < ph.filesz)
            goto bad;
        if(ph.vaddr + ph.memsz < ph.vaddr)
            goto bad;
        if((ph.vaddr % PGSIZE) != 0)
            goto bad;
        program_size = ph.off;
        for(j = 0; j < ph.filesz; j += PGSIZE){
            uint n; 
            if(ph.filesz - j < PGSIZE)
                n = ph.filesz - j;
            else 
                n = PGSIZE;

            if(readi(ip, pa, ph.off + j, n) != n){
                printk("%s %d: readi program failed\n", __func__, __LINE__);
                goto bad;
            }

            /*覆盖旧进程的代码段，其实并没有真正覆盖，只是重新分配了基地址页表，重新建立了映射*/
            if(vm_map_program(pagetable, ph.off + i, pa, n) < 0){
                printk("%s %d: vm_map_program failed\n", __func__, __LINE__);
                goto bad;
            }
            program_size += PGSIZE;
        }
    }
    elf.entry += vma->vm_start;
    //update program vma->vm_end
    vma->vm_end = program_size + vma->vm_start;
    

    //分配一个stack的VMA
    vma = vm_area_alloc(current->mm);
    vma->vm_end = PAGE_ALIGN(STACK_TOP_MAX);
    vma->vm_start = (vma->vm_end - PGSIZE) & PAGE_MASK;
    vma->vm_flags = VM_STACK_FLAGS;

    memset((void *)pa, 0, PGSIZE);
    sp = pa;
    stackbase = sp - PGSIZE;
    for( argc = 0; argv[argc]; argc++) {
        if(argc >= MAXARG)
            goto bad;
        sp -= strlen(argv[argc]) + 1;
        sp -= sp % 16;// riscv sp必须16字节对齐
        if(sp < stackbase)
            goto bad;
        //存放每个参数的数据到栈上
        memcpy(sp, argv[argc], strlen(argv[argc]) + 1); 
        //将参数的虚拟地址暂时放在stack数组中
        stack[argc] = sp - stackbase + vma->vm_start;
    }
    stack[argc] = 0;

    // push the array of argv[] pointers.
    sp -= (argc+1) * sizeof(uint64);
    sp -= sp % 16;
    memcpy(sp, (char *)stack, (argc+1)*sizeof(uint64));

    p->thread.sp  = sp - stackbase + vma->vm_start;
    for(last = s = path; *s; s++){
        if(*s == '/')
            last = s+1;
    }
    safestrcpy(p->name, last, sizeof(p->name));
    //map栈空间
    vm_map_normal_mem(pagetable, vma->vm_start, (char *)pa, PGSIZE);

    p->thread.ra = elf.entry;
    p->user_sp = sp - stackbase + vma->vm_start;
    //这里需要释放旧进程的代码段内存空间(vma释放)和栈空间(vma释放)，堆空间(vma释放)

    struct vm_area_struct *old_vma = current->mm->mmap;
    pagetable_t old_pagetable = current->mm->pagetable;
    current->mm->pagetable = pagetable;
    free_pagtable_and_vma(old_pagetable, old_vma);
    return 0;
bad:

    return 0;
}
