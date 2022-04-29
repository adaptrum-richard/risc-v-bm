#include "spinlock.h"
#include "sleeplock.h"
#include "sched.h"
#include "proc.h"

void initsleeplock(struct sleeplock *lk, char *name)
{
    initlock(&lk->lk, "sleep lock");
    lk->name = name;
    lk->locked = 0;
    lk->pid = 0;
}

void acquiresleep(struct sleeplock *lk)
{
    acquire(&lk->lk);
    while(lk->locked){
        release(&lk->lk);
        wait((uint64)lk);
        acquire(&lk->lk);
    }
    lk->locked = 1;
    lk->pid = current->pid;
    release(&lk->lk);
}

void releasesleep(struct sleeplock *lk)
{
    acquire(&lk->lk);
    lk->locked = 0;
    lk->pid = 0;
    wake((uint64)lk);
    release(&lk->lk);
}

int holdingsleep(struct sleeplock *lk)
{
    int r;
    acquire(&lk->lk);
    r = lk->locked && (lk->pid == current->pid);
    release(&lk->lk);
    return r;
}
