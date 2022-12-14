#include "spinlock.h"
#include "sched.h"
#include "preempt.h"
#include "riscv.h"
#include "printk.h"

void initlock(struct spinlock *lk, char *name)
{
    lk->name = name;
    lk->owner = 1;
    lk->next = 0;
    lk->cpu = -1;
}


static inline void  sync_synchronize()
{
    asm volatile("fence rw, rw");
}

static void unlock(struct spinlock *lock)
{
    ++(lock->owner);
    __smp_wmb();
}

/*将new值原子写入*locked，然后返回locked的旧值*/
static inline void lock(struct spinlock *lock)
{
    unsigned long times = 0;
#ifdef ZCU102
    struct spinlock lockval = *lock;
    lockval.next++;
#else
    unsigned long tmp;
    struct spinlock lockval;
    asm volatile(
        "1:\n"
        "lr.w %[new_next], (%[lock_next])\n"
        "lw %[new_owner], (%[lock_owner])\n"
        "addi %[new_next], %[new_next], 1\n"
        "sc.w %[tmp],%[new_next],(%[lock_next])\n"
        "bnez %[tmp], 1b\n"
        :[new_next]"=r"(lockval.next), [new_owner]"=r"(lockval.owner), [tmp]"=r"(tmp)
        :[lock_next]"r"(&lock->next), [lock_owner]"r"(&lock->owner)
        :"memory"
    );
#endif
    times = jiffies + HZ*2;
    while(lockval.next != lockval.owner){
        wfe();
        if(timer_after(jiffies, times))
            panic("wait timeout, lock name:%s\n", lock->name);
        lockval.owner = READ_ONCE(lock->owner);
    }
#ifdef ZCU102
    lock->next++;
#endif
}

/*将new值原子写入*locked，然后返回locked的旧值*/
static inline int try_lock(struct spinlock *lock)
{
    unsigned long tmp;
    int ret = -1;
    struct spinlock lockval;
    asm volatile(
        "1:\n"
        "lr.w %[new_next], (%[lock_next])\n"
        "lw %[new_owner], (%[lock_owner])\n"
        "bne %[new_owner], %[new_next], 2f\n"
        "addi %[new_next], %[new_next], 1\n"
        "addi %[ret], %[ret], 1\n"
        "sc.w %[tmp],%[new_next],(%[lock_next])\n"
        "bnez %2, 1b\n"
        "2:"
        :[new_next]"=r"(lockval.next), [new_owner]"=r"(lockval.owner), [tmp]"=r"(tmp), [ret]"=r"(ret)
        :[lock_next]"r"(&lock->next), [lock_owner]"r"(&lock->owner)
        :"memory"
    );
    return ret;
}

int try_acquire(struct spinlock *lk)
{
    preempt_disable();
    if(try_lock(lk) == 0){
        __smp_rmb();
        return 0;
    }
    preempt_enable();
    return -1;
}

void acquire(struct spinlock *lk)
{
    preempt_disable();
    lock(lk);
    lk->cpu = smp_processor_id();
    __smp_rmb();
}

void release(struct spinlock *lk)
{
    lk->cpu = -1;
    unlock(lk);
    preempt_enable();
    __smp_wmb();
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
    unlock(lock);
    arch_local_irq_restore(flags);
    preempt_enable();
}

void spin_unlock_irqrestore(struct spinlock *lock, unsigned long flags)
{
    __raw_spin_unlock_irqrestore(lock, flags);
}



