#include "elf.h"
#include "riscv.h"
#include "types.h"
#include "memlayout.h"
#include "printk.h"
#include "fs.h"
#include "vm.h"
#include "proc.h"
#include "log.h"

int loadseg(pagetable_t pagetable, uint64 va, struct inode *ip, uint offset, uint sz)
{
    uint i, n;
    uint64 pa;

    for (i = 0; i < sz; i += PGSIZE){
        pa = walkaddr(pagetable, va + i);
        if (pa == 0)
            panic("loadseg: address should exist");
        if (sz - i < PGSIZE)
            n = sz - i;
        else
            n = PGSIZE;
        if (readi(ip, (uint64)pa, offset + i, n) != n)
            return -1;
    }

    return 0;
}

int exec(char *path, char **argv)
{
    struct inode *ip;
    pagetable_t pagetable = 0;
    struct elfhdr elf;
    if(!path)
        return -1;
    
    log_begin_op();
    if((ip = namei(path)) == 0){
        log_end_op();
        return -1;
    }

    ilock(ip);
    if(readi(ip, (uint64)&elf, 0, sizeof(elf)) != sizeof(elf)){
        goto bad;
    }
    
    if(elf.magic != ELF_MAGIC)
        goto bad;
    
    /*创建一个空的基地址页表，并复制内核的页表*/
    if((pagetable = copy_kernel_tbl()) == 0){
        goto bad;
    }

    

bad:
    if(pagetable){

    }
    if(ip){
        iunlock(ip);
        log_end_op();
    }
    return -1;
}
