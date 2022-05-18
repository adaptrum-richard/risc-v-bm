#ifndef __RISCV_H__
#define __RISCV_H__
#include "types.h"

#define IRQ_S_SOFT		1
#define IRQ_S_TIMER		5
#define IRQ_S_EXT		  9

/* Exception causes */
#define EXC_INST_MISALIGNED         0
#define EXC_INST_ACCESS             1
#define EXC_INST_ILLEGAL            2
#define EXC_BREAKPOINT              3
#define EXC_LOAD_ADDRESS_MISALIGNED 4
#define EXC_LOAD_ACCESS             5
#define EXC_STORE_ACCESS            7
#define EXC_SYSCALL                 8
#define EXC_HYPERVISOR_SYSCALL      9
#define EXC_SUPERVISOR_SYSCALL      10
#define EXC_INST_PAGE_FAULT         12
#define EXC_LOAD_PAGE_FAULT         13
#define EXC_STORE_PAGE_FAULT        15
#define EXC_INST_GUEST_PAGE_FAULT   20
#define EXC_LOAD_GUEST_PAGE_FAULT   21
#define EXC_VIRTUAL_INST_FAULT      22
#define EXC_STORE_GUEST_PAGE_FAULT  23

#define PGSIZE 4096 // bytes per page
#define PGSHIFT 12  // bits of offset within a page

#define PGROUNDUP(sz)  (((sz)+PGSIZE-1) & ~(PGSIZE-1))
#define PGROUNDDOWN(a) (((a)) & ~(PGSIZE-1))

static inline uint64 r_stval()
{
  uint64 x;
  asm volatile("csrr %0, stval" : "=r" (x) );
  return x;
}

static inline void w_sepc(uint64 x)
{
  asm volatile("csrw sepc, %0" : : "r" (x));
}

static inline uint64 r_sepc()
{
  uint64 x;
  asm volatile("csrr %0, sepc" : "=r" (x) );
  return x;
}

static inline uint64 r_mhartid()
{
  uint64 x;
  asm volatile("csrr %0, mhartid" : "=r" (x) );
  return x;
}

// Supervisor Status Register, sstatus

#define SSTATUS_SPP (1L << 8)  // Previous mode, 1=Supervisor, 0=User
#define SSTATUS_SPIE (1L << 5) // Supervisor Previous Interrupt Enable
#define SSTATUS_UPIE (1L << 4) // User Previous Interrupt Enable
#define SSTATUS_SIE (1L << 1)  // Supervisor Interrupt Enable
#define SSTATUS_UIE (1L << 0)  // User Interrupt Enable

// Supervisor Interrupt Pending
static inline uint64 r_sip()
{
  uint64 x;
  asm volatile("csrr %0, sip" : "=r" (x) );
  return x;
}

static inline void w_sip(uint64 x)
{
  asm volatile("csrw sip, %0" : : "r" (x));
}

// Supervisor Trap Cause
static inline uint64 r_scause()
{
  uint64 x;
  asm volatile("csrr %0, scause" : "=r" (x) );
  return x;
}

static inline void w_stvec(uint64 x)
{
  asm volatile("csrw stvec, %0" : : "r" (x));
}

static inline uint64 r_sstatus()
{
  uint64 x;
  asm volatile("csrr %0, sstatus" : "=r" (x) );
  return x;
}

static inline void  w_sstatus(uint64 x)
{
  asm volatile("csrw sstatus, %0" : : "r" (x));
}

static inline int intr_get()
{
  uint64 x = r_sstatus();
  return (x & SSTATUS_SIE) != 0;
}

// enable device interrupts
static inline void intr_on()
{
  w_sstatus(r_sstatus() | SSTATUS_SIE);
}

// disable device interrupts
static inline void intr_off()
{
  w_sstatus(r_sstatus() & ~SSTATUS_SIE);
}

// Machine Status Register, mstatus

#define MSTATUS_MPP_MASK (3L << 11) // previous mode.
#define MSTATUS_MPP_M (3L << 11)
#define MSTATUS_MPP_S (1L << 11)
#define MSTATUS_MPP_U (0L << 11)
#define MSTATUS_MIE (1L << 3)    // machine-mode interrupt enable.


static inline uint64 r_mstatus()
{
    uint64 x;
    asm volatile("csrr %0, mstatus" : "=r" (x));
    return x;
}

static inline void w_mstatus(uint64 x)
{
    asm volatile("csrw mstatus, %0" :: "r" (x));
}

static inline uint64 r_tp()
{
  uint64 x;
  asm volatile("mv %0, tp" : "=r" (x) );
  return x;
}

static inline void w_tp(uint64 x)
{
  asm volatile("mv tp, %0" : : "r" (x));
}

/*
mepc:机器模式异常程序计数器( Machine Exception Program Counter)
当trap进入 M 模式时，mepc 被写入被中断或遇到异常的指令的虚拟地址。
*/
static inline void w_mepc(uint64 x)
{
  asm volatile("csrw mepc, %0" : : "r" (x));
}

/*
satp保存页表基地址
*/
static inline void w_satp(uint64 x)
{
  asm volatile("csrw satp, %0" : : "r" (x));
}

static inline void w_medeleg(uint64 x)
{
  asm volatile("csrw medeleg, %0" : : "r" (x));
}

// Machine Interrupt Delegation
static inline uint64 r_mideleg()
{
  uint64 x;
  asm volatile("csrr %0, mideleg" : "=r" (x) );
  return x;
}

static inline void  w_mideleg(uint64 x)
{
  asm volatile("csrw mideleg, %0" : : "r" (x));
}

// Machine-mode Interrupt Enable
#define MIE_MEIE (1L << 11) // external
#define MIE_MTIE (1L << 7)  // timer
#define MIE_MSIE (1L << 3)  // software
static inline uint64
r_mie()
{
  uint64 x;
  asm volatile("csrr %0, mie" : "=r" (x) );
  return x;
}

// Supervisor Interrupt Enable
#define SIE_SEIE (1L << 9) // external
#define SIE_STIE (1L << 5) // timer
#define SIE_SSIE (1L << 1) // software
static inline uint64
r_sie()
{
  uint64 x;
  asm volatile("csrr %0, sie" : "=r" (x) );
  return x;
}

static inline void 
w_sie(uint64 x)
{
  asm volatile("csrw sie, %0" : : "r" (x));
}

static inline void
w_pmpcfg0(uint64 x)
{
  asm volatile("csrw pmpcfg0, %0" : : "r" (x));
}

static inline void
w_pmpaddr0(uint64 x)
{
  asm volatile("csrw pmpaddr0, %0" : : "r" (x));
}

// Machine-mode interrupt vector
static inline void 
w_mtvec(uint64 x)
{
  asm volatile("csrw mtvec, %0" : : "r" (x));
}

static inline void 
w_mie(uint64 x)
{
  asm volatile("csrw mie, %0" : : "r" (x));
}

// Supervisor Scratch register, for early trap handler in trampoline.S.
static inline void 
w_sscratch(uint64 x)
{
  asm volatile("csrw sscratch, %0" : : "r" (x));
}

static inline void 
w_mscratch(uint64 x)
{
  asm volatile("csrw mscratch, %0" : : "r" (x));
}

typedef uint64 pte_t;
typedef uint64 *pagetable_t; // 512 PTEs

// extract the three 9-bit page table indices from a virtual address.
#define PXMASK          0x1FF // 9 bits
#define PXSHIFT(level)  (PGSHIFT+(9*(level)))
#define PX(level, va)   ((((uint64) (va)) >> PXSHIFT(level)) & PXMASK)

// one beyond the highest possible virtual address.
// MAXVA is actually one bit less than the max allowed by
// Sv39, to avoid having to sign-extend virtual addresses
// that have the high bit set.
#define MAXVA (1L << (9 + 9 + 9 + 12 - 1))

#define PTE_V (1L << 0) // valid
#define PTE_R (1L << 1)
#define PTE_W (1L << 2)
#define PTE_X (1L << 3)
#define PTE_U (1L << 4) // 1 -> user can access

// shift a physical address to the right place for a PTE.
#define PA2PTE(pa) ((((uint64)pa) >> 12) << 10)

#define PTE2PA(pte) (((pte) >> 10) << 12)

// flush the TLB.
static inline void sfence_vma()
{
  // the zero, zero means flush all TLB entries.
  asm volatile("sfence.vma zero, zero");
}

// use riscv's sv39 page table scheme.
#define SATP_SV39 (8L << 60)

#define MAKE_SATP(pagetable) (SATP_SV39 | (((uint64)pagetable) >> 12))

#endif
