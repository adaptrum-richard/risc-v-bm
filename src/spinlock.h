#ifndef __SPINLOCK_H__
#define __SPINLOCK_H__
#include "types.h"

// Mutual exclusion lock.
struct spinlock {
  uint locked;       // Is the lock held?

  // For debugging:
  char *name;        // Name of lock.
};

#define spin_lock_irqsave(lock, flags)  \
do {        \
    flags = __raw_spin_lock_irqsave(lock);  \
}while(0)   

void spin_unlock_irqrestore(struct spinlock *lock, 
  unsigned long flags);

void release(struct spinlock *lk);
void acquire(struct spinlock *lk);
void initlock(struct spinlock *lk, char *name);
int try_acquire(struct spinlock *lk);
void release_irq(struct spinlock *lk);
void acquire_irq(struct spinlock *lk);
#endif
