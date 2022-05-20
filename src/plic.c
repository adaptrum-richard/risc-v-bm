#include "memlayout.h"
#include "types.h"
#include "plic.h"
#include "proc.h"

void plicinithart(void)
{
  int hart = smp_processor_id();
  
  // set uart's enable bit for this hart's S-mode. 
  *(uint32*)PLIC_SENABLE(hart)= (1 << UART0_IRQ) | (1 << VIRTIO0_IRQ);

  // set this hart's S-mode priority threshold to 0.
  *(uint32*)PLIC_SPRIORITY(hart) = 0;
}

// ask the PLIC what interrupt we should serve.
int plic_claim(void)
{
    int hart = smp_processor_id();
    int irq = *(uint32*)PLIC_SCLAIM(hart);
    return irq;
}

// tell the PLIC we've served this IRQ.
void plic_complete(int irq)
{
  int hart = smp_processor_id();
  *(uint32*)PLIC_SCLAIM(hart) = irq;
}


void plicinit(void)
{
    // set desired IRQ priorities non-zero (otherwise disabled).
    *(uint32*)(PLIC + UART0_IRQ*4) = 1;
    *(uint32*)(PLIC + VIRTIO0_IRQ*4) = 1;
}
