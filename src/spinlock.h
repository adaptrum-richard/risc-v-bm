#ifndef __SPINLOCK_H__
#define __SPINLOCK_H__
#include "types.h"

// Mutual exclusion lock.
struct spinlock {
  uint locked;       // Is the lock held?

  // For debugging:
  char *name;        // Name of lock.
};
void release(struct spinlock *lk);
void acquire(struct spinlock *lk);
void initlock(struct spinlock *lk, char *name);
void release_irq(struct spinlock *lk);
void acquire_irq(struct spinlock *lk);
#endif
