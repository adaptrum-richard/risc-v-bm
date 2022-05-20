#include "spinlock.h"
#include "sched.h"
#include "preempt.h"
#include "riscv.h"

void initlock(struct spinlock *lk, char *name)
{
    lk->name = name;
    lk->locked = 0;
}
static inline void  sync_synchronize()
{
    asm volatile("fence rw, rw");
}

static void unlock(uint *lock)
{
    uint tmp0= 0, tmp1=0;
    //sync_synchronize();
    asm volatile(
        "amoswap.w.rl %0, %2, %1\n"
        : "=r"(tmp0), "+A"(*lock)
        : "r"(tmp1)
        : "memory");
}

/*将new值原子写入*locked，然后返回locked的旧值*/
static inline uint lock(uint *lock, uint new)
{
    uint org;
    asm volatile(
        "amoswap.w.aq %0, %2, %1\n"
        : "=r"(org), "+A"(*lock)
        : "r"(new)
        : "memory");
    return org;
}

void acquire(struct spinlock *lk)
{
    preempt_disable();
    while (lock(&lk->locked, 1) != 0)
        ;
}

int try_acquire(struct spinlock *lk)
{
    preempt_disable();
    if(lock(&lk->locked, 1) == 0){
        return true;
    }
    preempt_enable();
    return false;
};


void release(struct spinlock *lk)
{
    unlock(&lk->locked);
    preempt_enable();
}

void acquire_irq(struct spinlock *lk)
{
    intr_off();
    preempt_disable();
    while (lock(&lk->locked, 1) != 0)
        ;
}

void release_irq(struct spinlock *lk)
{
    unlock(&lk->locked);
    preempt_enable();
    intr_on();
}

static inline void arch_local_irq_restore(unsigned long flags)
{
    csr_set(sstatus, flags & SSTATUS_SIE);
}

static inline void __raw_spin_unlock_irqrestore(struct spinlock *lock,
    unsigned long flags)
{
    unlock(&lock->locked);
    arch_local_irq_restore(flags);
    preempt_enable();
}

void spin_unlock_irqrestore(struct spinlock *lock, unsigned long flags)
{
    __raw_spin_unlock_irqrestore(lock, flags);
}



