// Physical memory layout

// qemu -machine virt is set up like this,
// based on qemu's hw/riscv/virt.c:
//
// 00001000 -- boot ROM, provided by qemu
// 02000000 -- CLINT
// 0C000000 -- PLIC
// 10000000 -- uart0 
// 10001000 -- virtio disk 
// 80000000 -- boot ROM jumps here in machine mode
//             -kernel loads the kernel here
// unused RAM after 80000000.

// the kernel uses physical memory thus:
// 80000000 -- entry.S, then kernel text and data
// end -- start of kernel page allocation area
// PHYSTOP -- end RAM used by the kernel

// qemu puts UART registers here in physical memory.
#ifndef __MEMLAOUT_H__
#define __MEMLAOUT_H__
#ifdef ZCU102

#define UART_BASE   0x30000000L
#define UART0       0x30000100L
#define UART0_IRQ   0

#else

#define UART0 0x10000000L
#define UART0_IRQ 10

#endif

#define E1000_IRQ 33

// virtio mmio interface
#define VIRTIO0 0x10001000
#define VIRTIO0_IRQ 1

// core local interruptor (CLINT), which contains the timer.
#define CLINT 0x2000000L
#define CLINT_MTIMECMP(hartid) (CLINT + 0x4000 + 8*(hartid))
#define CLINT_MTIME (CLINT + 0xBFF8) // cycles since boot.

// qemu puts platform-level interrupt controller (PLIC) here.
#define PLIC 0x0c000000L
#define PLIC_PRIORITY (PLIC + 0x0)
#define PLIC_PENDING (PLIC + 0x1000)
#define PLIC_MENABLE(hart) (PLIC + 0x2000 + (hart)*0x100)
#define PLIC_SENABLE(hart) (PLIC + 0x2080 + (hart)*0x100)
#define PLIC_MPRIORITY(hart) (PLIC + 0x200000 + (hart)*0x2000)
#define PLIC_SPRIORITY(hart) (PLIC + 0x201000 + (hart)*0x2000)
#define PLIC_MCLAIM(hart) (PLIC + 0x200004 + (hart)*0x2000)
#define PLIC_SCLAIM(hart) (PLIC + 0x201004 + (hart)*0x2000)

// the kernel expects there to be RAM
// for use by the kernel and user pages
// from physical address 0x80000000 to PHYSTOP.
#define KERNBASE 0x80000000L
#define PHYSTOP (KERNBASE + 128*1024*1024)

// map the trampoline page to the highest address,
// in both user and kernel space.
#define TRAMPOLINE (MAXVA - PGSIZE)

// map kernel stacks beneath the trampoline,
// each surrounded by invalid guard pages.
#define KSTACK(p) (TRAMPOLINE - ((p)+1)* 2*PGSIZE)

// User memory layout.
// Address zero first:
//   text
//   original data and bss
//   fixed-size stack
//   expandable heap
//   ...
//   TRAPFRAME (p->trapframe, used by the trampoline)
//   TRAMPOLINE (the same page as in the kernel)
#define TRAPFRAME (TRAMPOLINE - PGSIZE)

#define ARCH_PHYS_OFFSET (KERNBASE)

/*
https://docs.kernel.org/translations/zh_CN/riscv/vm-layout.html
RISC-V Linux Kernel SV39
========================================================================================================================
    开始地址       |   偏移      |     结束地址      |  大小    | 虚拟内存区域描述
========================================================================================================================
                  |            |                  |         |
 0000000000000000 |    0       | 0000003fffffffff |  256 GB | 用户空间虚拟内存，每个内存管理器不同
__________________|____________|__________________|_________|___________________________________________________________
                  |            |                  |         |
 0000004000000000 | +256    GB | ffffffbfffffffff | ~16M TB | ... 巨大的、几乎64位宽的直到内核映射的-256GB地方
                  |            |                  |         |     开始偏移的非经典虚拟内存地址空洞。
                  |            |                  |         |
__________________|____________|__________________|_________|___________________________________________________________
                                                            |
                                                            | 内核空间的虚拟内存，在所有进程之间共享:
____________________________________________________________|___________________________________________________________
                  |            |                  |         |
 ffffffc6fee00000 | -228    GB | ffffffc6feffffff |    2 MB | fixmap
 ffffffc6ff000000 | -228    GB | ffffffc6ffffffff |   16 MB | PCI io
 ffffffc700000000 | -228    GB | ffffffc7ffffffff |    4 GB | vmemmap
 ffffffc800000000 | -224    GB | ffffffd7ffffffff |   64 GB | vmalloc/ioremap space
 ffffffd800000000 | -160    GB | fffffff6ffffffff |  124 GB | 直接映射所有物理内存
 fffffff700000000 |  -36    GB | fffffffeffffffff |   32 GB | kasan
__________________|____________|__________________|_________|____________________________________________________________
                                                            |
                                                            |
____________________________________________________________|____________________________________________________________
                  |            |                  |         |
 ffffffff00000000 |   -4    GB | ffffffff7fffffff |    2 GB | modules, BPF
 ffffffff80000000 |   -2    GB | ffffffffffffffff |    2 GB | kernel
__________________|____________|__________________|_________|____________________________________________________________
*/
#define UVM_START (0)
#define UVM_END (0x3fffffffff)

#define PGSIZE 4096 // bytes per page
#define PGSHIFT 12  // bits of offset within a page
/*我们只有3级页表*/
#define PGDIR_SHIFT_L3  30
#define PGDIR_SHIFT     PGDIR_SHIFT_L3
#define PGDIR_SIZE      (1UL << PGDIR_SHIFT)
#define PTRS_PER_PGD    (PGSIZE / sizeof(pte_t))

#define TASK_SIZE      (PGDIR_SIZE * PTRS_PER_PGD / 2)
#define STACK_TOP		TASK_SIZE //0x4000000000 刚好是用户空间地址的最顶端 254G
#define STACK_TOP_MAX	(STACK_TOP)
#define STACK_ALIGN		16

#define USER_CODE_VM_START 0x1000000000 //64GB

#define USER_MEM_START ((1UL<< 32)*8 + USER_CODE_VM_START)     //72GB
#define USER_MEM_END (USER_MEM_START + (1UL << 32)*30)   //102GB

#define STACK_BOTTOM (STACK_TOP_MAX - (10UL << 32) ) //224G

#endif
