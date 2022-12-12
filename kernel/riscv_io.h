#ifndef __RISV_IO_H__
#define __RISV_IO_H__
#include "types.h"
#define __io_br()	do {} while (0)
#define __io_ar()	__asm__ __volatile__ ("fence i,r" : : : "memory");
#define __io_bw()	__asm__ __volatile__ ("fence w,o" : : : "memory");
#define __io_aw()	do {} while (0)

static inline uint32 __raw_readl(const volatile void *addr)
{
	uint32 val;

	asm volatile("lw %0, 0(%1)" : "=r"(val) : "r"(addr));
	return val;
}


static inline void __raw_writel(uint32 val, volatile void *addr)
{
	asm volatile("sw %0, 0(%1)" : : "r"(val), "r"(addr));
}

#define writel(v,c)	({ __io_bw(); __raw_writel((v),(c)); __io_aw(); })
#define readl(c)	({ uint32 __v; __io_br(); __v = __raw_readl(c); __io_ar(); __v; })

#endif
