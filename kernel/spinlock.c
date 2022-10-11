#include "spinlock.h"
#include "sched.h"
#include "preempt.h"
#include "riscv.h"
#include "printk.h"

void initlock(struct spinlock *lk, char *name)
{
    lk->name = name;
    lk->locked = 0;
    lk->cpu = -1;
}

int holding(struct spinlock *lk)
{
    int r;
    r = (lk->locked && lk->cpu == smp_processor_id());
    return r;
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
extern volatile int init_done_flag;
void acquire(struct spinlock *lk)
{
    preempt_disable();
    if(holding(lk))
        panic("acquire");
    while(__sync_lock_test_and_set(&lk->locked, 1) != 0)
        ;
    lk->cpu = smp_processor_id();
    __sync_synchronize();
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
    //unlock(&lk->locked);
    lk->cpu = -1;
    __sync_synchronize();
    __sync_lock_release(&lk->locked);
    preempt_enable();
}

void acquire_irq(struct spinlock *lk)
{
    intr_off();
    acquire(lk);
}

void release_irq(struct spinlock *lk)
{
    release(lk);
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



