#include "ip_app.h"
#include "net.h"
#include "types.h"
#include "debug.h"
#include "spinlock.h"
#include "sched.h"
#include "timer.h"

static ipaddr_t local_ip = MAKE_IP_ADDR(10, 0, 2, 15); // qemu's idea of the guest IP
static struct eth_addr local_mac = { {0x52, 0x54, 0x00, 0x12, 0x34, 0x56} };
static struct eth_addr broadcast_mac = {{0xFF, 0XFF, 0XFF, 0XFF, 0XFF, 0XFF}};

static ipaddr_t broadcast_ipaddr = MAKE_IP_ADDR(0xff,0xff,0xff,0xff);



arp_waitqueue_t arp_wq_head = {
    .entry = LIST_HEAD_INIT(arp_wq_head.entry),
    .p = NULL,
    .ipaddr = MAKE_IP_ADDR(0, 0, 0, 0),
    .lock = INIT_SPINLOCK("arp_wq")
};

int init_arp_waitqueue_entry(arp_waitqueue_t *arpwq, ipaddr_t ipaddr)
{
    arpwq->entry.next = arpwq->entry.prev = NULL;
    initlock(&arpwq->lock, "arp_wq_entry");
    ipaddr_copy(arpwq->ipaddr, ipaddr);
    return 0;
}

int ip_app_add_arp_wq_entry(arp_waitqueue_t *wq_entry)
{
    unsigned long flags;
    if(on_arp_waitqueue(wq_entry)){
        printk("%s %d: bug\n", __func__, __LINE__);
        return 0;
    }
    spin_lock_irqsave(&arp_wq_head.lock, flags);
    wq_entry->p = current;
    list_add_tail(&wq_entry->entry, &arp_wq_head.entry);
    spin_unlock_irqrestore(&arp_wq_head.lock, flags);
    return 0;
}

static int ip_app_del_arp_wq_entry(arp_waitqueue_t *wq_entry)
{
    if(!on_arp_waitqueue(wq_entry)){
        printk("%s %d: bug\n", __func__, __LINE__);
        return 0;
    }
    if(list_empty(&arp_wq_head.entry)){
        printk("%s %d: ERROR\n", __func__, __LINE__);
        return -1;
    }

    list_del(&wq_entry->entry);
    if(wq_entry->p)
        wake_up_process(wq_entry->p);
    return 0;
}

static int ip_app_arp_wq_try_wakeup_proccess_and_del_entry(ipaddr_t ipaddr)
{
    unsigned long flags;
    int found = 0;
    arp_waitqueue_t *wq_entry;
    spin_lock_irqsave(&arp_wq_head.lock, flags);
    list_for_each_entry(wq_entry, &arp_wq_head.entry, entry){
       if(ipaddr_cmp(ipaddr, wq_entry->ipaddr)){
            found = 1;
            break;
       }
    }
    if(found == 1)
        ip_app_del_arp_wq_entry(wq_entry);
    spin_unlock_irqrestore(&arp_wq_head.lock, flags);
    return 0;
}

int ip_app_wait_arp_reply(arp_waitqueue_t *wq_entry, ipaddr_t ipaddr, signed timeout)
{
    init_arp_waitqueue_entry(wq_entry, ipaddr);
    ip_app_add_arp_wq_entry(wq_entry);
    return schedule_timeout(timeout);
}

int ip_app_wake_arp_process(ipaddr_t ipaddr)
{
    return ip_app_arp_wq_try_wakeup_proccess_and_del_entry(ipaddr);
}

ipaddr_t ip_app_get_broadcast_ipaddr()
{
    return broadcast_ipaddr;
}

struct eth_addr *ip_app_get_broadcast_mac(void)
{
    return &broadcast_mac;
}

void ip_app_set_local_ip(ipaddr_t *ipaddr)
{
    if(!ipaddr){
        pr_err("set default local ipaddr failed\n");
        printk("use default ipaddr: ");
        print_ipaddr(&local_ip);
        printk("\n");
        return;
    }
    ipaddr_copy(local_ip, *ipaddr);
}

ipaddr_t ip_app_get_local_ip(void)
{
    return local_ip;
}

struct eth_addr *ip_app_get_local_mac(void)
{
    return &local_mac;
}


void print_ipaddr(ipaddr_t *ipaddr)
{
    if(ipaddr)
        printk("%d.%d.%d.%d", 
            (*ipaddr >> 24) & 0xff,
            (*ipaddr >> 16) & 0xff,
            (*ipaddr >> 8) & 0xff,
            *ipaddr & 0xff);
}

void print_mac(struct eth_addr *mac)
{
    if(mac)
        printk("%02x:%02x:%02x:%02x:%02x:%02x",
            mac->addr[0], 
            mac->addr[1],
            mac->addr[2],
            mac->addr[3],
            mac->addr[4],
            mac->addr[5]);
}
