#ifndef __JIFFIES_H__
#define __JIFFIES_H__
#include "typecheck.h"

#define HZ 250
#ifdef ZCU102
//#define CPU_FREQ 300000000UL //300MHz
#define CPU_FREQ   1000000UL //50MHz
#else
#define CPU_FREQ 10000000UL
#endif
/*一般a参数是为jiffies，b参数是timeout的时间。*/

//time_after(a,b) 返回 true，如果时间a在时间b的后面
#define timer_after(a, b) \
    (typecheck(unsigned long, a) && \
      typecheck(unsigned long, b) && \
      (long)((b) - (a)) < 0)

/*timer_before(a,b) 返回 true，如果时间a在时间b的前面*/
#define timer_before(a, b) timer_after(b, a)

/*timer_after_eq(a,b)返回true, a>=b*/
#define timer_after_eq(a, b) \
    (typecheck(unsigned long, a) && \
      typecheck(unsigned long, b) && \
      (long)((a) - (b)) >= 0)

/*timer_after_eq(a,b)返回true, a<=b*/
#define timer_before_eq(a, b) timer_after_eq(b, a)
extern volatile uint64 jiffies;
#endif
