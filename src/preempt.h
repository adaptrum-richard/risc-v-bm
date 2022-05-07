#ifndef __PREEMPT_H__
#define __PREEMPT_H__
#include "proc.h"


/*
PREEMPT_MASK:	0x000000ff
SOFTIRQ_MASK:	0x0000ff00
HARDIRQ_MASK:	0x000f0000
*/
#define PREEMPT_BITS	8
#define SOFTIRQ_BITS	8
#define HARDIRQ_BITS	4

#define PREEMPT_SHIFT	0
#define SOFTIRQ_SHIFT	(PREEMPT_SHIFT + PREEMPT_BITS)
#define HARDIRQ_SHIFT	(SOFTIRQ_SHIFT + SOFTIRQ_BITS)

#define __IRQ_MASK(x)	((1UL << (x))-1)

#define PREEMPT_MASK	(__IRQ_MASK(PREEMPT_BITS) << PREEMPT_SHIFT)
#define SOFTIRQ_MASK	(__IRQ_MASK(SOFTIRQ_BITS) << SOFTIRQ_SHIFT)
#define HARDIRQ_MASK	(__IRQ_MASK(HARDIRQ_BITS) << HARDIRQ_SHIFT)


#define PREEMPT_OFFSET	(1UL << PREEMPT_SHIFT)
#define SOFTIRQ_OFFSET	(1UL << SOFTIRQ_SHIFT)
#define HARDIRQ_OFFSET	(1UL << HARDIRQ_SHIFT)

static inline int preempt_count(void)
{
    return (int)current->preempt_count;
}

#define hardirq_count()	(preempt_count() & HARDIRQ_MASK)
#define softirq_count()	(preempt_count() & SOFTIRQ_MASK)
#define irq_count()	(hardirq_count() | softirq_count())

/*
in_serving_softirq()	- We're in softirq context
in_hardirq()		- We're in hard IRQ context
in_task()		- We're in task context
*/
#define in_serving_softirq()	(softirq_count() & SOFTIRQ_OFFSET)
#define in_hardirq()		(hardirq_count())
#define in_task()		(in_hardirq() | in_serving_softirq()))



static inline void preempt_disable(void)
{
    current->preempt_count++;
}

static inline void preempt_enable(void)
{
    current->preempt_count--;
}

#endif
