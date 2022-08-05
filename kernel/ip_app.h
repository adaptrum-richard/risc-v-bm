#ifndef __IP_APP_H__
#define __IP_APP_H__
#include "net.h"
#include "proc.h"
#include "list.h"
#include "spinlock.h"
#include "timer.h"

typedef struct event_timeout_wq{
    struct task_struct *p;
    unsigned long condition;
    struct spinlock lock;
    struct list_head entry;
    struct timer_list timer;
}event_timeout_wq_t;


#define ipaddr_netcmp(addr1, addr2, mask) ((addr1 & mask) == (addr2 & mask))
#define on_event_timeout_wq(wq) (wq->entry.next != NULL)

void print_ipaddr(ipaddr_t *ipaddr);
ipaddr_t ip_app_get_local_ip(void);
struct eth_addr *ip_app_get_local_mac(void);
struct eth_addr *ip_app_get_broadcast_mac(void);
ipaddr_t ip_app_get_broadcast_ipaddr();
void print_mac(struct eth_addr *mac);

ipaddr_t ip_app_get_local_netmask(void);
void ip_app_set_local_netmask(ipaddr_t *ipaddr);

ipaddr_t ip_app_get_local_gateway(void);
void ip_app_set_local_gateway(ipaddr_t *ipaddr);

ipaddr_t ip_app_get_local_broadcast(void);
void ip_app_set_local_broadcast(ipaddr_t *ipaddr);

/*如果等待超时，则返回0，
未超时，则返回剩余等待的时间 timeout的单位是一个jiffies*/
long ip_app_wq_timeout_wait_condion(unsigned long condition, signed timeout);
/*唤醒等待arp响应的进程*/
int ip_app_wq_wakeup_process(unsigned long condition);
#endif
