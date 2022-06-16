#include "wait.h"
#include "proc.h"
#include "sched.h"

#define WAITQUEUE_WALK_BREAK_CNT 64

static int __wake_up_common(struct wait_queue_head *wq_head, unsigned int mode,
                            int nr_exclusive, int wake_flags, void *key,
                            wait_queue_entry_t *bookmark)
{
    wait_queue_entry_t *curr, *next;
    int cnt = 0;

    if (bookmark && (bookmark->flags & WQ_FLAG_BOOKMARK)){
        curr = list_next_entry(bookmark, entry);

        list_del(&bookmark->entry);
        bookmark->flags = 0;
    }
    else
        curr = list_first_entry(&wq_head->head, wait_queue_entry_t, entry);

    if (&curr->entry == &wq_head->head)
        return nr_exclusive;

    list_for_each_entry_safe_from(curr, next, &wq_head->head, entry){
        unsigned flags = curr->flags;
        int ret;

        if (flags & WQ_FLAG_BOOKMARK)
            continue;

        ret = curr->func(curr, mode, wake_flags, key);
        if (ret < 0)
            break;
        /**/
        if (ret && (flags & WQ_FLAG_EXCLUSIVE) && !--nr_exclusive)
            break;

        if (bookmark && (++cnt > WAITQUEUE_WALK_BREAK_CNT) &&
            (&next->entry != &wq_head->head))
        {
            bookmark->flags = WQ_FLAG_BOOKMARK;
            list_add_tail(&bookmark->entry, &next->entry);
            break;
        }
    }
    return nr_exclusive;
}

static void __wake_up_common_lock(struct wait_queue_head *wq_head, unsigned int mode,
                                  int nr_exclusive, int wake_flags, void *key)
{
    unsigned long flags;
    wait_queue_entry_t bookmark;
    bookmark.flags = 0;
    bookmark.private = NULL;
    bookmark.func = NULL;
    INIT_LIST_HEAD(&bookmark.entry);
    do
    {
        spin_lock_irqsave(&wq_head->lock, flags);
        nr_exclusive = __wake_up_common(wq_head, mode, nr_exclusive,
                                        wake_flags, key, &bookmark);
        spin_unlock_irqrestore(&wq_head->lock, flags);
    } while (bookmark.flags & WQ_FLAG_BOOKMARK);
}

void __wake_up(struct wait_queue_head *wq_head, unsigned int mode,
               int nr_exclusive, void *key)
{
    __wake_up_common_lock(wq_head, mode, nr_exclusive, 0, key);
}

long prepare_to_wait_event(struct wait_queue_head *wq_head, 
            struct wait_queue_entry *wq_entry, int state)
{
    unsigned long flags;
    long ret = 0;
    spin_lock_irqsave(&wq_head->lock, flags);
    
    if (list_empty(&wq_entry->entry)) {
        /*如果entry是空，说明没有加入到等待队列*/
        /*加入到等待队列*/
        if(wq_entry->flags & WQ_FLAG_EXCLUSIVE)
            __add_wait_queue_entry_tail(wq_head, wq_entry);
        else
            __add_wait_queue(wq_head, wq_entry);

        set_current_state(state);
    }
    spin_unlock_irqrestore(&wq_head->lock, flags);
    return ret;
}

int autoremove_wake_function(struct wait_queue_entry *wq_entry, unsigned mode,
     int sync, void *key)
{
    int ret = default_wake_function(wq_entry, mode, sync, key);
    if(ret)
        list_del_init_careful(&wq_entry->entry);
    return ret;
}

void init_wait_entry(struct wait_queue_entry *wq_entry, int flags)
{
    wq_entry->flags = flags;
    wq_entry->private = current;
    wq_entry->func = autoremove_wake_function;
    INIT_LIST_HEAD(&wq_entry->entry);
}

void finish_wait(struct wait_queue_head *wq_head, 
            struct wait_queue_entry *wq_entry)
{
    unsigned long flags;
    set_current_state(TASK_RUNNING);
    if(!list_empty_careful(&wq_entry->entry)) {
        spin_lock_irqsave(&wq_head->lock, flags);
        list_del_init(&wq_entry->entry);
        spin_unlock_irqrestore(&wq_head->lock, flags);
    }
}
