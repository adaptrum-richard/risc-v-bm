#include "ip_app.h"
#include "net.h"
#include "types.h"
#include "debug.h"

static ipaddr_t local_ip = MAKE_IP_ADDR(10, 0, 2, 15); // qemu's idea of the guest IP
static struct eth_addr local_mac = { {0x52, 0x54, 0x00, 0x12, 0x34, 0x56} };
static struct eth_addr broadcast_mac = {{0xFF, 0XFF, 0XFF, 0XFF, 0XFF, 0XFF}};

static ipaddr_t broadcast_ipaddr = MAKE_IP_ADDR(0xff,0xff,0xff,0xff);

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
