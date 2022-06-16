#ifndef __LIST_H__
#define __LIST_H__
#include "types.h"
#include "barrier.h"

#define LIST_HEAD_INIT(name) {&(name), &(name)}

#define LIST_HEAD(name) \
    struct list_head name = LIST_HEAD_INIT(name)

static inline void INIT_LIST_HEAD(struct list_head *list)
{
    WRITE_ONCE(list->next, list);
    list->prev = list;
}

static inline int list_is_head(const struct list_head *list, 
        const struct list_head *head)
{
    return list == head;
}
static inline void __list_add(struct list_head *new,
                            struct list_head *prev,
                            struct list_head *next)
{
    next->prev = new;
    new->next = next;
    new->prev = prev;
    WRITE_ONCE(prev->next, new);
}

/*
new增加到head后面
*/
static inline void list_add(struct list_head *new, struct list_head *head)
{
    __list_add(new, head, head->next);
}

static inline void list_add_tail(struct list_head *new, struct list_head *head)
{
    __list_add(new, head->prev, head);
}

static inline void __list_del(struct list_head *prev, struct list_head *next)
{
    next->prev = prev;
    WRITE_ONCE(prev->next, next);
}

static inline void __list_del_entry(struct list_head *entry)
{
    __list_del(entry->prev, entry->next);
}


static inline void list_del(struct list_head *entry)
{
    __list_del_entry(entry);
    entry->next = NULL;
    entry->prev = NULL;
}

static inline void list_replace(struct list_head *old,
                            struct list_head *new)
{
    new->next = old->next;
    new->next->prev = new;
    new->prev = old->prev;
    new->prev->next = new;
}

static inline int list_empty(const struct list_head *head)
{
    return READ_ONCE(head->next) == head;
}

#define container_of(ptr, type, member) ({  \
    ((type *)((char*)ptr - offsetof(type, member))); })

#define list_entry(ptr, type, member) \
    container_of(ptr, type, member)

#define list_first_entry(ptr, type, member) \
    list_entry((ptr)->next, type, member)

#define list_last_entry(ptr, type, member) \
    list_entry((ptr)->prev, type, member)

#define list_next_entry(pos, member) \
    list_entry((pos)->member.next, typeof(*(pos)), member)

#define list_prev_entry(pos, member) \
    list_entry((pos)->member.prev, typeof(*(pos)), member)

#define list_for_each(pos, head) \
    for (pos = (head)->next; !list_is_head(pos, (head)); pos = pos->next)

#define list_for_each_safe(pos, n, head) \
    for (pos = (head)->next, n = pos->next; \
        !list_is_head(pos, (head)); \
            pos = n, n = pos->next)

#define list_entry_is_head(pos, head, member)   \
        (&pos->member == (head))

#define list_for_each_entry(pos, head, member)  \
    for (pos = list_first_entry(head, typeof(*pos), member);	\
        !list_entry_is_head(pos, head, member);	    \
        pos = list_next_entry(pos, member))

#define list_for_each_entry_safe_from(pos, n, head, member) 			\
    for (n = list_next_entry(pos, member);					\
    !list_entry_is_head(pos, head, member);				\
        pos = n, n = list_next_entry(n, member))

static inline void list_del_init_careful(struct list_head *entry)
{
    __list_del_entry(entry);
    entry->prev = entry;
    smp_store_release(&entry->next, entry);
}

static inline int list_empty_careful(const struct list_head *head)
{
    struct list_head *next = smp_load_acquire(&head->next);
    return list_is_head(next, head) && (next == head->prev);
}

static inline void list_del_init(struct list_head *entry)
{
    __list_del_entry(entry);
    INIT_LIST_HEAD(entry);
}
#endif
