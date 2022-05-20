#include "spinlock.h"
#include "sleeplock.h"
#include "sched.h"
#include "proc.h"
#include "wait.h"

DECLARE_WAIT_QUEUE_HEAD(sleeplock_wait_queue);

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
    //while(lk->locked){
    release(&lk->lk);
    wait_event(sleeplock_wait_queue, READ_ONCE(lk->locked) == 0);
    acquire(&lk->lk);
    //}
    lk->locked = 1;
    lk->pid = current->pid;
    release(&lk->lk);
}

void releasesleep(struct sleeplock *lk)
{
    acquire(&lk->lk);
    lk->locked = 0;
    lk->pid = 0;
    wake_up(&sleeplock_wait_queue);
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
