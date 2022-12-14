#include "memlayout.h"
#include "types.h"
#include "plic.h"
#include "proc.h"

void plicinithart(void)
{
    int hart = smp_processor_id();
#ifndef ZCU102
    // set uart's enable bit for this hart's S-mode.
    *(uint32 *)PLIC_SENABLE(hart) = (1 << UART0_IRQ) | (1 << VIRTIO0_IRQ);
    // hack to get at next 32 IRQs for e1000
    *(uint32*)(PLIC_SENABLE(hart)+4) = 0xffffffff;
    // set this hart's S-mode priority threshold to 0.
    *(uint32 *)PLIC_SPRIORITY(hart) = 0;
#else
    *(uint32 *)PLIC_SENABLE(hart) = (0xffffffff);
    
    *(uint32*)(PLIC_SENABLE(hart)+4) = 0xffffffff;
    *(uint32*)(PLIC_SENABLE(hart)+8) = 0xffffffff;
    *(uint32*)(PLIC_SENABLE(hart)+12) = 0xffffffff;

    *(uint32 *)PLIC_MENABLE(hart) = (0xffffffff);
    *(uint32 *)(PLIC_MENABLE(hart)+4) = (0xffffffff);
    *(uint32 *)(PLIC_MENABLE(hart)+8) = (0xffffffff);
    *(uint32 *)(PLIC_MENABLE(hart)+12) = (0xffffffff);
    *(uint32 *)PLIC_SPRIORITY(hart) = 0;
    *(uint32 *)PLIC_MPRIORITY(hart) = 0;

    *(uint32 *)PLIC_SENABLE(1) = (0xffffffff);
    *(uint32 *)PLIC_MENABLE(1) = (0xffffffff);
    
    *(uint32 *)PLIC_SPRIORITY(1) = 0;
    *(uint32 *)PLIC_MPRIORITY(1) = 0;

    *(uint32 *)PLIC_SENABLE(2) = (0xffffffff);
    *(uint32 *)PLIC_MENABLE(2) = (0xffffffff);
    
    *(uint32 *)PLIC_SPRIORITY(2) = 0;
    *(uint32 *)PLIC_MPRIORITY(2) = 0;

    *(uint32 *)PLIC_SENABLE(3) = (0xffffffff);
    *(uint32 *)PLIC_MENABLE(3) = (0xffffffff);
    
    *(uint32 *)PLIC_SPRIORITY(3) = 0;
    *(uint32 *)PLIC_MPRIORITY(3) = 0;

    *(uint32 *)PLIC_SENABLE(4) = (0xffffffff);
    *(uint32 *)PLIC_MENABLE(4) = (0xffffffff);
    
    *(uint32 *)PLIC_SPRIORITY(4) = 0;
    *(uint32 *)PLIC_MPRIORITY(4) = 0;
#endif
}

// ask the PLIC what interrupt we should serve.
int plic_claim(void)
{
    int hart = smp_processor_id();
    int irq = *(uint32 *)PLIC_SCLAIM(hart);
    return irq;
}

#ifdef ZCU102
int plic_m_claim(void)
{
    int hart = smp_processor_id();
    int irq = *(uint32 *)PLIC_SCLAIM(hart);
    return irq;
}
#endif

// tell the PLIC we've served this IRQ.
void plic_complete(int irq)
{
    int hart = smp_processor_id();
    *(uint32 *)PLIC_SCLAIM(hart) = irq;
}

void plicinit(void)
{
    // set desired IRQ priorities non-zero (otherwise disabled).
    *(uint32 *)(PLIC + UART0_IRQ * 4) = 1;
#ifndef ZCU102
    *(uint32 *)(PLIC + VIRTIO0_IRQ * 4) = 1;

    // PCIE IRQs are 32 to 35
    for(int irq = 1; irq < 0x35; irq++){
        *(uint32*)(PLIC + irq*4) = 1;
    }
#endif
}
