#ifndef __WAIT_H__
#define __WAIT_H__
#include "list.h"
#include "spinlock.h"
/*
https://www.cnblogs.com/hueyxu/p/13745029.html
以进程阻塞和唤醒的过程为例，等待队列的使用场景可以简述为：
进程 A 因等待某些资源（依赖进程 B 的某些操作）而不得不进入阻塞状态，
便将当前进程加入到等待队列 Q 中。进程 B 在一系列操作后，可通知进程 A 
所需资源已到位，便调用唤醒函数 wake up 来唤醒等待队列上 Q 的进程，注
意此时所有等待在队列 Q 上的进程均被置为可运行状态。
*/


/*
WQ_FLAG_EXCLUSIVE:
当某进程调用 wake up 函数唤醒等待队列时，队列上所有的进程均被唤醒，
在某些场合会出现唤醒的所有进程中，只有某个进程获得了期望的资源，而
其他进程由于资源被占用不得不再次进入休眠。如果等待队列中进程数量庞
大时，该行为将影响系统性能。
内核增加了"独占等待”(WQ_FLAG_EXCLUSIVE)来解决此类问题。一个独占等
待的行为和通常的休眠类似，但有如下两个重要的不同：
1.等待队列元素设置 WQ_FLAG_EXCLUSIVE 标志时，会被添加到等
待队列的尾部，而非头部。
2.在某等待队列上调用 wake up 时，执行独占等待的进程每次只会唤醒其
中第一个（所有非独占等待进程仍会被同时唤醒）。
*/
#define WQ_FLAG_EXCLUSIVE	0x01

#define WQ_FLAG_WOKEN		0x02

/*
WQ_FLAG_BOOKMARK:
用于 wake_up() 唤醒等待队列时实现分段遍历，减少单次对自旋锁的占用时间。
*/
#define WQ_FLAG_BOOKMARK	0x04
#define WQ_FLAG_CUSTOM		0x08
#define WQ_FLAG_DONE		0x10
#define WQ_FLAG_PRIORITY	0x20

struct task_struct;

typedef struct wait_queue_entry wait_queue_entry_t;
typedef int (*wait_queue_func_t)(struct wait_queue_entry *wq_entry, 
        unsigned mode, int flags, void *key);

int default_wake_function(struct wait_queue_entry *wq_entry, 
    unsigned mode, int flags, void *key);

struct wait_queue_entry {
    unsigned int        flags;
    void                *private;
    wait_queue_func_t   func;
    struct list_head    entry;
};

struct wait_queue_head {
    struct spinlock     lock;
    struct list_head    head;
};
typedef struct wait_queue_head wait_queue_head_t;

long prepare_to_wait_event(struct wait_queue_head *wq_head, 
            struct wait_queue_entry *wq_entry, int state);
void finish_wait(struct wait_queue_head *wq_head, 
            struct wait_queue_entry *wq_entry);
void init_wait_entry(struct wait_queue_entry *wq_entry, int flags);
void __wake_up(struct wait_queue_head *wq_head, unsigned int mode,
               int nr_exclusive, void *key);
               
static inline void init_waitqueue_entry(struct wait_queue_entry *wq_entry, 
                    struct task_struct *p)
{
    wq_entry->flags = 0;
    wq_entry->private = p;
    wq_entry->func = default_wake_function;
}

static inline int waitqueue_active(struct wait_queue_head *wq_head)
{
    return !list_empty(&wq_head->head);
}

static inline void __add_wait_queue_entry_tail(struct wait_queue_head *wq_head, 
    struct wait_queue_entry *wq_entry)
{
    list_add_tail(&wq_entry->entry, &wq_head->head);
}

static inline void __add_wait_queue(struct wait_queue_head *wq_head, 
            struct wait_queue_entry *wq_entry)
{
    struct list_head *head = &wq_head->head;
    struct wait_queue_entry *wq;
    list_for_each_entry(wq, &wq_head->head, entry) {
        if (!(wq->flags & WQ_FLAG_PRIORITY))
            break;
        head = &wq->entry;
    }
    list_add(&wq_entry->entry, head);
}

/*
只唤醒一个threads
*/
static inline void _add_wait_queue_exclusive(struct wait_queue_head *wq_head, 
            struct wait_queue_entry *wq_entry)
{
    wq_entry->flags |= WQ_FLAG_EXCLUSIVE;
    __add_wait_queue(wq_head, wq_entry);
}

static inline void __remove_wait_queue(struct wait_queue_head *wq_head, 
            struct wait_queue_entry *wq_entry)
{
    list_del(&wq_entry->entry);
}

#define ___wait_is_interruptible(state) \
        (state == TASK_INTERRUPTIBLE)   


/*TODO:未检测KILL信号和被中断挂起的情况*/
#define ___wait_event(wq_head, condition, state, exclusive, ret, cmd)		\
({ \
    struct wait_queue_entry __wq_entry;    \
    init_wait_entry(&__wq_entry, exclusive ? WQ_FLAG_EXCLUSIVE : 0); \
    for (;;) { \
        prepare_to_wait_event(&wq_head, &__wq_entry, state); \
        if (condition) \
            break; \
        cmd;    \
    }   \
    finish_wait(&wq_head, &__wq_entry); \
})
        

#define __wait_event(wq_head, condition)    \
    (void)___wait_event(wq_head, condition, TASK_UNINTERRUPTIBLE, 0, 0,	\
        schedule())

#define wait_event(wq_head, condition)  \
do {   \
    if (condition) \
        break;  \
    __wait_event(wq_head, condition); \
}while(0)

#define __wait_event_interruptible(wq_head, condition) \
    ___wait_event(wq_head, condition, TASK_INTERRUPTIBLE, 0, 0, schedule()) 

#define wait_event_interruptible(wq_head, condition) \
do {    \
    if(condition) \
        break; \
    ___wait_event(wq_head, condition, TASK_INTERRUPTIBLE, 0, 0,	schedule()); \
}while(0)

#define poll_to_key(m) ((void*)m)

#define wake_up_poll(x, m) \
    __wake_up(x, TASK_NORMAL, 1, poll_to_key(m))

#define __WAIT_QUEUE_HEAD_INITIALIZER(name) { \
    .lock		= INIT_SPINLOCK(name), \
    .head		= LIST_HEAD_INIT(name.head)  \
}

#define wake_up(x)  \
    __wake_up(x, TASK_NORMAL, 1, NULL)


#define DECLARE_WAIT_QUEUE_HEAD(name) \
    struct wait_queue_head name = __WAIT_QUEUE_HEAD_INITIALIZER(name)

#endif
