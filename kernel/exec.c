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
#include "string.h"
#include "slab.h"
#include "fork.h"
#include "spinlock.h"
#include "debug.h"

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
    int i, off;
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

    for(i = 0, off = elf.phoff; i < elf.phnum; i++, off += sizeof(ph)){

        if (readi(ip, (uint64)&ph, off, sizeof(ph)) != sizeof(ph)){
            printk("%s %d: read program header failed\n", __func__, __LINE__);
            goto bad;
        }
        /*riscv64-linux-gnu-gcc 11.2.0或riscv64-unknown-linux-gnu 11.1.0 在编译initcode时，
        会多出一个RISCV_ATTRIBUT段
        $ riscv64-linux-gnu-readelf -l ./user/initcode.out 

        Elf 文件类型为 EXEC (可执行文件)
        Entry point 0x1000000000
        There are 2 program headers, starting at offset 64

        程序头：
        Type           Offset             VirtAddr           PhysAddr
                        FileSiz            MemSiz              Flags  Align
        RISCV_ATTRIBUT 0x00000000000000f9 0x0000000000000000 0x0000000000000000
                        0x000000000000002e 0x0000000000000000  R      0x1
        LOAD           0x00000000000000b0 0x0000001000000000 0x0000001000000000
                        0x0000000000000049 0x0000000000000049  RWE    0x10

        Section to Segment mapping:
        段节...
        00     .riscv.attributes 
        01     .text 
        */
        if(ph.type != ELF_PROG_LOAD)
            continue;
        if (ph.vaddr + ph.memsz < ph.vaddr){
            printk("%s %d: read program header, memsz: 0x%llx, vaddr:0x%llx\n", 
                __func__, __LINE__, ph.memsz, ph.vaddr);
            goto bad;
        }
        //这里只有一个program段，所以读取到后直接返回
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
    }
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
*/

int exec(char *path, char **argv)
{    
    int i, off, j, ret;

    struct elfhdr elf;
    struct proghdr ph;
    struct elfshdr shdr;
    
    struct inode *ip;
    struct vm_area_struct *pg_vma = NULL, *stack_vma = NULL;
    uint64 pa;
    uint64 vm_prg_end = 0;
    unsigned long start, end;
    uint64 argc = 0;
    char *s, *last;
    uint64 sp, stackbase, stack[MAXARG];
    
    //首先判断文件是否存在
    if(path == NULL)
        return -1;

    log_begin_op();
    if((ip = namei(path)) == 0){
        log_end_op();
        return -1;
    }
    log_end_op();
    iput(ip);
    /*1. 找到当前进程的stack的vma*/
    stack_vma = find_vma(current->mm, STACK_TOP - 1);
    pr_exec_debug("to exec:[%2s], current name:[%2s]\n", 
        path, current->name);
    
    if(!stack_vma){
        //1.1. 如果当前进程的vma不存在，分配一个stack的VMA
        stack_vma = vm_area_alloc(current->mm);
        //初始化vma
        vm_area_init(stack_vma, (stack_vma->vm_end - PGSIZE) & PAGE_MASK,
            PAGE_ALIGN(STACK_TOP_MAX), VM_STACK_FLAGS);
        pr_exec_debug("alloc stack,vma addr:0x%lx, vm [0x%lx-0x%lx]\n",
            (unsigned long)stack_vma, stack_vma->vm_start, stack_vma->vm_end);
    }else {
        //1.2 如果当前进程的vma存在，释放stack的所有物理页面
        unmap_validpages(current->mm->pagetable, stack_vma->vm_start, 
            (stack_vma->vm_end - stack_vma->vm_start) / PGSIZE);
        pr_exec_debug("unmap stack,vma addr:0x%lx, vm [0x%lx-0x%lx]\n",
            (unsigned long)stack_vma, stack_vma->vm_start, stack_vma->vm_end);
        stack_vma->vm_start = STACK_TOP - PGSIZE;
        stack_vma->vm_end = STACK_TOP;
        pr_exec_debug("reset stack,vma addr:0x%lx, vm [0x%lx-0x%lx]\n",
            (unsigned long)stack_vma, stack_vma->vm_start, stack_vma->vm_end);
    }
    /*
     * 1.3 根据stack虚拟地址找到物理地址
     *  如果stack的虚拟地址不存在，则分配一个物理页面作为存放栈上数据
     */
    pa = walkaddr(current->mm->pagetable, STACK_TOP - 1);
    if(!pa)
        pa = get_free_page();
    else
        pr_exec_err("bug\n");
    //1.4 将栈数据存放在pa上
    sp = pa + PGSIZE;
    stackbase = pa;
    for( argc = 0; argv[argc] != NULL; argc++) {
        if(argc >= MAXARG)
            goto bad;
        
        sp -= (strlen(argv[argc]) + 1);
        sp -= sp % 16;// riscv sp必须16字节对齐
        if(sp < stackbase)
            goto bad;
        //存放每个参数的数据到栈上
        spcae_data_copy_in((void*)sp, (unsigned long)argv[argc], strlen(argv[argc]) + 1);

        //将参数的虚拟地址暂时放在stack数组中
        stack[argc] = sp - stackbase + STACK_TOP - PGSIZE;
        pr_exec_debug("argv[%d]: %s, addr:0x%lx\n", argc, argv[argc], stack[argc]);
    }

    stack[argc] = 0;
    // push the array of argv[] pointers.
    sp -= (argc+1) * sizeof(uint64);
    sp -= sp % 16;
    memcpy((void *)sp, (char *)stack, (argc+1)*sizeof(uint64));

    for(last = s = path; *s; s++){
        if(*s == '/')
            last = s+1;
    }
    safestrcpy(current->name, last, sizeof(current->name));
    set_user_mode_sp(current, sp - stackbase + stack_vma->vm_start);
    set_user_mode_a1(current, sp - stackbase + stack_vma->vm_start);
    /*TODO:释放掉多余stack, 这里我们只使用了一个页面，在以前映射的多余的page需要被unmap，并释放掉*/
    start = stack_vma->vm_start;
    end = STACK_TOP - PGSIZE;
    if(start < end){
        pr_exec_err("bug\n");
        unmap_validpages(current->mm->pagetable, start, (end - start) / PGSIZE);
        pr_exec_debug("unmap remaining stack: [0x%lx-0x%lx]\n",
            start, start+(end - start) / PGSIZE);
    }
    //更新vm_start
    stack_vma->vm_start = end;
    //1.5 将刚新分配的pa映射到stack_vma上
    pr_exec_debug("mmap stackbase:0x%lx size:0x%x to va [%lx]\n", 
        stackbase, PGSIZE, stack_vma->vm_start);
    ret = vm_map_normal_mem(current->mm->pagetable, stack_vma->vm_start, (unsigned char *)stackbase, PGSIZE);
    if(ret < 0)
        goto bad;
    free_page(pa);
    pa = 0;
    pr_exec_info("run\n");
    log_begin_op();
    if((ip = namei(path)) == 0){
        log_end_op();
        printk("%s %d: namei path '%s' failed\n", __func__, __LINE__, path);
        return -1;
    }

    pr_exec_info("run\n");
    /*
     * 2. 用户程序使用malloc分配的内存
     */
    remove_vma_region(current->mm->mmap, USER_MEM_START, USER_MEM_END);
    pr_exec_info("run\n");
    /*
     * 3. 释放旧进程的代码段 bss段等数据
     */
    remove_vma_region(current->mm->mmap, USER_CODE_VM_START, USER_MEM_START);
    pr_exec_info("run\n");
    /*
     4. Load program into memory.
    */
    pg_vma  = vm_area_alloc(current->mm);
    pg_vma->vm_start = USER_CODE_VM_START;
    pa = 0;

    ilock(ip);
    if(readi(ip, (uint64)&elf, 0, sizeof(elf))!= sizeof(elf)){
        printk("%s %d: readi elf failed\n", __func__, __LINE__);
        goto bad;
    }
    pa = get_free_page();
    for(i = 0, off = elf.phoff; i < elf.phnum; i++, off += sizeof(ph)){            
        if(readi(ip, (uint64)&ph, off, sizeof(ph)) != sizeof(ph))
            goto bad;
        if(pa == 0)
            pa = get_free_page();
        if(ph.type != ELF_PROG_LOAD)
            continue;
        pr_exec_debug("ph size:%d, align:%d, off = %d\n", sizeof(ph), ph.align, off);
        pr_exec_debug("ph.type: 0x%x, ph vaddr:0x%lx, memsz 0x%lx, filesz = 0x%lx flags = 0x%lx, elf.phnum = %lu\n", 
            ph.type,  ph.vaddr, ph.memsz, ph.filesz, ph.flags, elf.phnum);
        if(ph.memsz < ph.filesz)
            goto bad;

        if(ph.vaddr + ph.memsz < ph.vaddr)
            goto bad;
        if((ph.vaddr % PGSIZE) != 0)
            goto bad;
        for(j = 0; j < ph.filesz; j += PGSIZE){
            uint n; 
            if(pa == 0)
                pa = get_free_page();
            if(ph.filesz - j < PGSIZE)
                n = ph.filesz - j;
            else 
                n = PGSIZE;
            memset((void*)pa, 0x0, PGSIZE);
            if(readi(ip, pa, ph.off + j, n) != n){
                printk("%s %d: readi program failed\n", __func__, __LINE__);
                goto bad;
            }
            /*4.1 映射代码段*/
            pr_exec_debug("mmap program code: copy code to va[0x%lx], size = 0x%lx\n", 
                ph.vaddr + j, n);
            ret = map_program_code(current->mm->pagetable, ph.vaddr + j, 
                (unsigned char *)pa, n);
            if(ret < 0) {
                pr_exec_err("vm_map_program failed\n");
                goto bad;
            } else if (ret == 1){
                free_page(pa);
                pa = 0;
            }
            vm_prg_end = PGROUNDUP(ph.vaddr + ph.filesz);
            
        }
    }
    pg_vma->vm_end = vm_prg_end;
    pr_exec_debug("program vm[0x%lx-0x%lx]\n", pg_vma->vm_start, pg_vma->vm_end);

    //映射bss段数据
    for(i = 0, off = elf.shoff; i < elf.shnum; i++, off += sizeof(shdr)){
        if(readi(ip, (uint64)&shdr, off, sizeof(shdr)) != sizeof(shdr)){
            goto bad;
        }
        if(shdr.sh_type != ELF_BSS_TYPE)
            continue;
        pr_exec_debug("%s %d: sh_type = 0x%x, sh_size = 0x%lx, sh_addr = 0x%lx, flags = 0x%lx\n", 
                    __func__, __LINE__, 
                    shdr.sh_type, 
                    shdr.sh_size,
                    shdr.sh_addr,
                    shdr.sh_flags);
           

        for(j = 0; j < shdr.sh_size; j += PGSIZE){
                   uint n; 
            if(shdr.sh_size - j < PGSIZE)
                n = shdr.sh_size - j;
            else 
                n = PGSIZE;
            pr_exec_debug("bss mmap: va[0x%lx], size = 0x%lx, only memset bss is 0\n", shdr.sh_addr + j, n);
            map_program_bss(current->mm->pagetable, shdr.sh_addr + j, NULL, n);

            pr_exec_debug("sh_type = 0x%x, sh_size = 0x%lx, sh_addr = 0x%lx, flags = 0x%lx\n", shdr.sh_type, 
                    shdr.sh_size,
                    shdr.sh_addr,
                    shdr.sh_flags);

            vm_prg_end = PGROUNDUP(shdr.sh_addr + shdr.sh_size);
        }
        
    }
    show_vma_info(current->mm->pagetable, current->mm->mmap);
    pr_exec_debug("pg vma[0x%lx-0x%lx]\n", pg_vma->vm_start, pg_vma->vm_end);
    insert_vm_struct(current->mm, pg_vma);
    pr_exec_debug("name:%s, elf.entry = 0x%lx\n", 
        current->name, elf.entry);
    set_user_mode_epc(current, elf.entry);

    show_vma_info(current->mm->pagetable, current->mm->mmap);

    /*TODO: 需要释放malloc分配的内存*/
    iunlockput(ip); 
    log_end_op();
    return argc;
bad:
    if(pa)
        free_page(pa);
    if(ip){
        iunlockput(ip);
        log_end_op();
    }
    return -1;
}
