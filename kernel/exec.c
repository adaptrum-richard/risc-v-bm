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
*/

int exec(char *path, char **argv)
{    
    int i, off, j, ret;

    struct elfhdr elf;
    struct proghdr ph;
    struct elfshdr shdr;
    
    struct inode *ip;
    struct vm_area_struct *pg_vma = NULL, *stack_vma = NULL, *bss_vma = NULL;
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

    stack_vma = find_vma(current->mm, STACK_TOP - 1);
    pr_debug("%s %d: exec name: %s, current name = %s\n", __func__, __LINE__, path, current->name);
    if(!stack_vma){
        //分配一个stack的VMA
        stack_vma = vm_area_alloc(current->mm);
        //初始化vma
        stack_vma->vm_end = PAGE_ALIGN(STACK_TOP_MAX);
        stack_vma->vm_start = (stack_vma->vm_end - PGSIZE) & PAGE_MASK;
        stack_vma->vm_flags = VM_STACK_FLAGS;
    }else {
        unmap_validpages(current->mm->pagetable, stack_vma->vm_start, 
            (stack_vma->vm_end - stack_vma->vm_start) / PGSIZE);
        stack_vma->vm_start = STACK_TOP - PGSIZE;
        stack_vma->vm_end = STACK_TOP;
    }
    pa = walkaddr(current->mm->pagetable, STACK_TOP - 1);
    if(!pa)
        pa = get_free_page();

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
        pr_debug("%s %d: argv[%d]: %s, addr:0x%lx\n", __func__, __LINE__, argc, argv[argc],stack[argc]);
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
    pr_debug("%s %d: set a1: 0x%lx\n", __func__, __LINE__, 
        sp - stackbase + stack_vma->vm_start);
    /*TODO:释放掉多余stack, 这里我们只使用了一个页面，在以前映射的多余的page需要被unmap，并释放掉*/
    start = stack_vma->vm_start;
    end = STACK_TOP - PGSIZE;
    if(start < end){
        unmap_validpages(current->mm->pagetable, start, (end - start) / PGSIZE);
    }
    //更新vm_start
    stack_vma->vm_start = end;
    //map栈空间
    ret = vm_map_normal_mem(current->mm->pagetable, stack_vma->vm_start, (unsigned char *)stackbase, PGSIZE);
    if(ret < 0)
        goto bad;
    else if(ret == 1){
        free_page(pa);
        pa = 0;
    }
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
    
    /*第一个vma是用来存放代码段的*/
    pg_vma = current->mm->mmap;
    if(pg_vma && pg_vma->vm_end < USER_MEM_START){
         unmap_validpages(current->mm->pagetable, pg_vma->vm_start, 
            (pg_vma->vm_end - pg_vma->vm_start) / PGSIZE);
    }
    if(pg_vma->vm_next){
        if(pg_vma->vm_next->vm_end < USER_MEM_START){
            bss_vma = pg_vma->vm_next;
            unmap_validpages(current->mm->pagetable, bss_vma->vm_start, 
                (bss_vma->vm_end - bss_vma->vm_start) / PGSIZE);   
        }
    }
    if(!bss_vma)
    {
        bss_vma = vm_area_alloc(current->mm);
        insert_vm_struct(current->mm, bss_vma);
        bss_vma->vm_flags = VM_WRITE | VM_READ;
        bss_vma->vm_start = pg_vma->vm_start;
        bss_vma->vm_end = pg_vma->vm_end;
    }

    //pg_vma->vm_end 当program的数据读完后在填写vm_end

    /*
    Load program into memory.
    这里分配的一个page:
    1. 在加载program时，暂时存放code，在vm_map_program函数中会将code复制到另外一个page
    2. 在stack中会使用这个page存放栈上数据，做映射，成功的话，此page不需要被释放
    */
    pg_vma->vm_start = USER_CODE_VM_START;
    for(i = 0, off = elf.phoff; i < elf.phnum; i++, off += sizeof(ph)){
        if(pa == 0)
            pa = get_free_page();
        pr_debug("%s %d: i = %d\n", __func__, __LINE__, i);
        if(readi(ip, (uint64)&ph, off, sizeof(ph)) != sizeof(ph))
            goto bad;
       
        if(ph.type != ELF_PROG_LOAD)
            continue;
        pr_debug("ph size:%d, align:%d, off = %d\n", sizeof(ph), ph.align, off);
        pr_debug("%s %d:ph.type: 0x%x, ph vaddr:0x%lx, memsz 0x%lx, filesz = 0x%lx flags = 0x%lx, elf.phnum = %lu\n", 
            __func__, __LINE__,ph.type,  ph.vaddr, ph.memsz, ph.filesz, ph.flags, elf.phnum);
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
            
            if(readi(ip, pa, ph.off + j, n) != n){
                printk("%s %d: readi program failed\n", __func__, __LINE__);
                goto bad;
            }
            /*覆盖旧进程的代码段, */
            pr_debug("%s %d: va = 0x%lx\n", __func__, __LINE__, ph.vaddr + j);
            ret = map_program_code(current->mm->pagetable, ph.vaddr + j, 
                (unsigned char *)pa, n);
            //ret = vm_map_program(current->mm->pagetable, ph.vaddr + j, 
            //    (unsigned char *)pa, n, perm_elf_prg_to_table(ph.flags));
            if(ret < 0) {
                printk("%s %d: vm_map_program failed\n", __func__, __LINE__);
                goto bad;
            } else if (ret == 1){
                free_page(pa);
                pa = 0;
            }
            
            vm_prg_end = PGROUNDUP(ph.vaddr + ph.filesz);
        }
    }
    pg_vma->vm_end = vm_prg_end;
    
    int bss_flags = 0;
   
    for(i = 0, off = elf.shoff; i < elf.shnum; i++, off += sizeof(shdr)){
        if(readi(ip, (uint64)&shdr, off, sizeof(shdr)) != sizeof(shdr)){
            goto bad;
        }
        if(shdr.sh_type != ELF_BSS_TYPE)
            continue;
        pr_debug("%s %d: sh_type = 0x%x, sh_size = 0x%lx, sh_addr = 0x%lx, flags = 0x%lx\n", 
                    __func__, __LINE__, 
                    shdr.sh_type, 
                    shdr.sh_size,
                    shdr.sh_addr,
                    shdr.sh_flags);
           

        bss_flags = 1;
        for(j = 0; j < shdr.sh_size; j += PGSIZE){
                   uint n; 
            if(shdr.sh_size - j < PGSIZE)
                n = shdr.sh_size - j;
            else 
                n = PGSIZE;
            map_program_bss(current->mm->pagetable, shdr.sh_addr + j, NULL, n);

            pr_debug("sh_type = 0x%x, sh_size = 0x%lx, sh_addr = 0x%lx, flags = 0x%lx\n", shdr.sh_type, 
                    shdr.sh_size,
                    shdr.sh_addr,
                    shdr.sh_flags);

            vm_prg_end = PGROUNDUP(shdr.sh_addr + shdr.sh_size);
        }
        
    }

    if(bss_flags == 1){
        bss_vma->vm_start = pg_vma->vm_end;
        bss_vma->vm_end = vm_prg_end;
    }else {
        remove_vma(bss_vma);
    }
        
    set_user_mode_epc(current, elf.entry);

    //print_all_vma(current->mm->pagetable, current->mm->mmap);
    pr_debug("%s %d, name:%s, elf.entry = 0x%lx\n", 
        __func__, __LINE__,
        current->name, elf.entry);
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
