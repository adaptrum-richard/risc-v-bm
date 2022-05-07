#ifndef __HARDIRQ_H__
#define __HARDIRQ_H__

#include "preempt.h"

#define __irq_enter_raw() \
    do {                \
        preempt_count_add(HARDIRQ_OFFSET); \
    }while(0)

#define __irq_exit_raw() \
    do {                \
        preempt_count_sub(HARDIRQ_OFFSET); \
    }while(0)


void irq_enter(void);
void irq_exit(void);
#endif