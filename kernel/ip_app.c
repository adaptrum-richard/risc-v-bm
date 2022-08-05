#include "ip_app.h"
#include "net.h"
#include "types.h"
#include "debug.h"
#include "spinlock.h"
#include "sched.h"
#include "timer.h"

static ipaddr_t local_ip = MAKE_IP_ADDR(10, 0, 2, 15); // qemu's idea of the guest IP
static ipaddr_t local_netmask = MAKE_IP_ADDR(0, 0, 0, 0);
static ipaddr_t local_gateway = MAKE_IP_ADDR(0, 0, 0, 0);
static ipaddr_t local_broadcast = MAKE_IP_ADDR(0, 0, 0, 0);
static struct eth_addr local_mac = { {0x52, 0x54, 0x00, 0x12, 0x34, 0x56} };
static struct eth_addr broadcast_mac = {{0xFF, 0XFF, 0XFF, 0XFF, 0XFF, 0XFF}};

static ipaddr_t broadcast_ipaddr = MAKE_IP_ADDR(0xff,0xff,0xff,0xff);

event_timeout_wq_t event_timeout_wq_head = {
    .entry = LIST_HEAD_INIT(event_timeout_wq_head.entry),
    .p = NULL,
    .condition = 0,
    .lock = INIT_SPINLOCK("event_timeout_wq")
};

int init_event_timeout_wq_entry(event_timeout_wq_t *e, unsigned long condition)
{
    e->entry.next = e->entry.prev = NULL;
    initlock(&e->lock, "event_timeout_wq_entry");
    e->condition = condition;
    return 0;
}

int ip_app_add_wq_entry(event_timeout_wq_t *wq_entry)
{
    unsigned long flags;
    if(on_event_timeout_wq(wq_entry)){
        printk("%s %d: bug\n", __func__, __LINE__);
        return 0;
    }
    spin_lock_irqsave(&event_timeout_wq_head.lock, flags);
    wq_entry->p = current;
    set_current_state(TASK_INTERRUPTIBLE);
    list_add_tail(&wq_entry->entry, &event_timeout_wq_head.entry);
    spin_unlock_irqrestore(&event_timeout_wq_head.lock, flags);
    return 0;
}

static int ip_app_del_wq_entry(event_timeout_wq_t *wq_entry)
{
    unsigned long flags;
    if(!on_event_timeout_wq(wq_entry)){
        return 0;
    }
    if(list_empty(&event_timeout_wq_head.entry)){
        printk("%s %d: ERROR\n", __func__, __LINE__);
        return -1;
    }
    spin_lock_irqsave(&event_timeout_wq_head.lock, flags);
    list_del(&wq_entry->entry);
    spin_unlock_irqrestore(&event_timeout_wq_head.lock, flags);
    return 0;
}

static int ip_app_wq_try_wakeup_proccess_and_del_entry(unsigned long condition)
{
    unsigned long flags;
    event_timeout_wq_t *curr;
    struct list_head *next, *prev;
    struct list_head tmp_head = LIST_HEAD_INIT(tmp_head);

    spin_lock_irqsave(&event_timeout_wq_head.lock, flags);
    list_for_each_safe(prev, next, &event_timeout_wq_head.entry){
        curr = list_entry(prev, event_timeout_wq_t, entry);
        if(condition == curr->condition){
            list_del(&curr->entry);
            list_add(&curr->entry, &tmp_head);
        }
    }
    
    if(list_empty(&tmp_head)){
        goto out;
    }
    /*wakeup process后应该立即从tmp_head中删除entry，
        否则在ip_app_del_wq_entry将会报错。原因是在没有超时的情况下，在此函数中将会先
        执行，被唤醒的进程回去执行ip_app_del_wq_entry来将entry从全局链表中删除
    */
    list_for_each_safe(prev, next,  &tmp_head){
        curr = list_entry(prev, event_timeout_wq_t, entry);
        if(curr->p){
            wake_up_process(curr->p);
            list_del(&curr->entry);
        }
    }
out:
    spin_unlock_irqrestore(&event_timeout_wq_head.lock, flags);
    return 0;
}

long ip_app_wq_timeout_wait_condition(unsigned long condition, signed timeout)
{
    event_timeout_wq_t wq_entry;
    long ret;
    init_event_timeout_wq_entry(&wq_entry, condition);
    ip_app_add_wq_entry(&wq_entry);
    ret = schedule_timeout(timeout);
    ip_app_del_wq_entry(&wq_entry);
    return ret;
}

int ip_app_wq_wakeup_process(unsigned long condition)
{
    return ip_app_wq_try_wakeup_proccess_and_del_entry(condition);
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

ipaddr_t ip_app_get_local_netmask(void)
{
    return local_netmask;
}

void ip_app_set_local_netmask(ipaddr_t *ipaddr)
{
    if(!ipaddr){
        pr_err("set default local netmask failed\n");
        printk("use default ipaddr: ");
        print_ipaddr(&local_ip);
        printk("\n");
        return;
    }
    ipaddr_copy(local_netmask, *ipaddr);
}

ipaddr_t ip_app_get_local_gateway(void)
{
    return local_gateway;
}

void ip_app_set_local_gateway(ipaddr_t *ipaddr)
{
    if(!ipaddr){
        pr_err("set default local gateway failed\n");
        printk("use default ipaddr: ");
        print_ipaddr(&local_ip);
        printk("\n");
        return;
    }
    ipaddr_copy(local_gateway, *ipaddr);
}

ipaddr_t ip_app_get_local_broadcast(void)
{
    return local_broadcast;
}

void ip_app_set_local_broadcast(ipaddr_t *ipaddr)
{
    if(!ipaddr){
        pr_err("set default local broadcas failed\n");
        printk("use default ipaddr: ");
        print_ipaddr(&local_ip);
        printk("\n");
        return;
    }
    ipaddr_copy(local_broadcast, *ipaddr);
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
