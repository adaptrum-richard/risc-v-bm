#ifndef __IP_APP_H__
#define __IP_APP_H__
#include "net.h"
#include "proc.h"
#include "list.h"
#include "spinlock.h"
#include "timer.h"

typedef struct arp_waitqueue{
    struct task_struct *p;
    ipaddr_t ipaddr;
    struct spinlock lock;
    struct list_head entry;
    struct timer_list timer;
}arp_waitqueue_t;

#define on_arp_waitqueue(arpwq) (arpwq->entry.next != NULL)

void print_ipaddr(ipaddr_t *ipaddr);
ipaddr_t ip_app_get_local_ip(void);
struct eth_addr *ip_app_get_local_mac(void);
struct eth_addr *ip_app_get_broadcast_mac(void);
ipaddr_t ip_app_get_broadcast_ipaddr();
void print_mac(struct eth_addr *mac);



/*发出arp后等待arp回应，如果等待超时，则返回0，
未超时，则返回剩余等待的时间 timeout的单位是一个jiffies*/
int ip_app_wait_arp_reply(arp_waitqueue_t *wq_entry, ipaddr_t ipaddr, signed timeout);

/*唤醒等待arp响应的进程*/
int ip_app_wake_arp_process(ipaddr_t ipaddr);
#endif
