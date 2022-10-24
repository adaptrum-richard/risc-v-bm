#include "param.h"
#include "riscv.h"
#include "memlayout.h"

/*保存m模式 timer中断的scratch*/
uint64 timer_scratch[NCPU][5];

__attribute__((aligned(16))) char stack0[4096 * NCPU];
void main();
void timerinit();
extern void timervec();
void start()
{
    unsigned long x = r_mstatus();
    x &= ~MSTATUS_MPP_MASK;
    x |= MSTATUS_MPP_S;
    w_mstatus(x);

    /*从m模式返回s模式时执行main函数*/
    w_mepc((uint64)main);

    /*禁止地址翻译*/
    w_satp(0);

    /*代理所有中断和异常到s模式, s-mode下调用ecall不被代理到s-mode。*/
    w_medeleg(0xffff & (~(1<<9)));
    w_mideleg(0xffff);

    /*开启s模式下的extension、异常、中断*/
    w_sie(r_sie() | SIE_SEIE | SIE_STIE | SIE_SSIE);

    // configure Physical Memory Protection to give supervisor mode
    // access to all of physical memory.
    /*0x3f_ffff_ffff_ffff
    A:01 TOR
    XRW均为1
    */
    w_pmpaddr0(0x3fffffffffffffull);
    w_pmpcfg0(0xf);

    // ask for clock interrupts.
    timerinit();

    // keep each CPU's hartid in its tp register, for cpuid().
    /*tp寄存器低4位用于保存cpuid，高60位用于保存进程task_struct地址*/
    int id = r_mhartid();
    w_tp(id);

    // switch to supervisor mode and jump to main().
    asm volatile("mret");
}

void timerinit()
{
    // each CPU has a separate source of timer interrupts.
    int id = r_mhartid();

    // ask the CLINT for a timer interrupt.
    /*在QEMU上，这个时钟的频率是10MHz, 每过1s, rdtime返回的结果增大10000000*/
    int interval = 100000; // cycles; about 1/100th second in qemu.
    *(uint64 *)CLINT_MTIMECMP(id) = *(uint64 *)CLINT_MTIME + interval;

    // prepare information in scratch[] for timervec.
    // scratch[0..2] : space for timervec to save registers.
    // scratch[3] : address of CLINT MTIMECMP register.
    // scratch[4] : desired interval (in cycles) between timer interrupts.
    uint64 *scratch = &timer_scratch[id][0];
    scratch[3] = CLINT_MTIMECMP(id);
    scratch[4] = interval;
    w_mscratch((uint64)scratch);

    // set the machine-mode trap handler.
    w_mtvec((uint64)timervec);

    // enable machine-mode interrupts.
    w_mstatus(r_mstatus() | MSTATUS_MIE);

    // enable machine-mode timer interrupts.
    w_mie(r_mie() | MIE_MTIE);
}
