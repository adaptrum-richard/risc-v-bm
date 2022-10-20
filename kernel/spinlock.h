#ifndef __SPINLOCK_H__
#define __SPINLOCK_H__
#include "types.h"
#include "riscv.h"
#include "typecheck.h"

// Mutual exclusion lock.
struct spinlock
{
    int owner; // Is the lock held?
    int next;
    // For debugging:
    char *name; // Name of lock.
    int cpu;
};

#define INIT_SPINLOCK(_name)  \
    {                         \
        .owner = 1,          \
        .next = 0,          \
        .name = "" #_name "", \
    }

void spin_unlock_irqrestore(struct spinlock *lock,
                            unsigned long flags);

void release(struct spinlock *lk);
void acquire(struct spinlock *lk);
void initlock(struct spinlock *lk, char *name);
void release_irq(struct spinlock *lk);
void acquire_irq(struct spinlock *lk);

static inline unsigned long arch_local_irq_save(void)
{
    return csr_read_clear(sstatus, SSTATUS_SIE);
}

#define raw_local_irq_save(flags)        \
    do                                   \
    {                                    \
        typecheck(unsigned long, flags); \
        flags = arch_local_irq_save();   \
    } while (0)

#define local_irq_save(flags)      \
    do                             \
    {                              \
        raw_local_irq_save(flags); \
    } while (0)

static inline unsigned long __raw_spin_lock_irqsave(struct spinlock *lock)
{
    unsigned long flags;
    local_irq_save(flags);
    acquire(lock);
    return flags;
}

#define spin_lock_irqsave(lock, flags)         \
    do                                         \
    {                                          \
        flags = __raw_spin_lock_irqsave(lock); \
    } while (0)

#endif
