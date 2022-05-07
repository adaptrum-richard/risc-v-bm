#ifndef __PREEMPT_H__
#define __PREEMPT_H__
#include "proc.h"
#include "types.h"

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
#define in_task()		(!(in_hardirq() | in_serving_softirq()))

/*判断是否在原子上下文*/
#define in_atomic()     (preempt_count() != 0)
static inline void __preempt_count_add(uint64 val)
{
    current->preempt_count += val;
}

static inline void __preempt_count_sub(uint64 val)
{
    current->preempt_count -= val;
}

#define preempt_count_add(val) __preempt_count_add(val)
#define preempt_count_sub(val) __preempt_count_sub(val)

static inline void preempt_disable(void)
{
    preempt_count_add(1);
}

static inline void preempt_enable(void)
{
    preempt_count_sub(1);
}

#endif
