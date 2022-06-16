#ifndef __PLIC_H__
#define __PLIC_H__

void plicinit(void);
void plic_complete(int irq);
int plic_claim(void);
void plicinithart(void);

#endif
