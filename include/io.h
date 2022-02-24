#ifndef __IO_H__
#define __IO_H__
#define __arch_getl(a)			(*(volatile unsigned int *)(a))
#define __arch_putl(v,a)		(*(volatile unsigned int *)(a) = (v))
#define dmb()		__asm__ __volatile__ ("" : : : "memory")
#define __iormb()	dmb()
#define __iowmb()	dmb()
#define writel(v,c)	({ unsigned int  __v = v; __iowmb(); __arch_putl(__v,c);})
#define readl(c)	({ unsigned int  __v = __arch_getl(c); __iormb(); __v; })

#define UART_DATA_ADDR 0x10000000
#endif