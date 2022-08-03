#include "ip_arp.h"
#include "debug.h"
#include "slab.h"
#include "net.h"
#include "string.h"
#include "spinlock.h"
#include "ip_app.h"
#include "ip_neighbor.h"

static struct arp_entry *p_arp_table;
static struct spinlock arp_lock;
static uint8 arptime;

void ip_arp_init(void)
{
    p_arp_table = kmalloc(sizeof(struct arp_entry)*ARP_ENTRIES);
    if(!p_arp_table){
        pr_err("%s failed\n", __func__);
        return;
    }
    memset(p_arp_table, 0, sizeof(struct arp_entry)*ARP_ENTRIES);
    initlock(&arp_lock, "arp");
    arptime = 0;
}

void arp_timer(void)
{
    int i;
    struct arp_entry *tabptr;
    acquire(&arp_lock);
    arptime++;
    for (i = 0; i < ARP_ENTRIES; ++i)
    {
        tabptr = &p_arp_table[i];
        if (tabptr->ipaddr != 0 && (arptime - tabptr->time) >= ARP_MAXAGE_TIME)
            memset(&tabptr->ipaddr, 0, sizeof(tabptr->ipaddr));
    }
    release(&arp_lock);
}

static void arp_update(ipaddr_t ipaddr, struct eth_addr *ethaddr)
{
    int i;
    struct arp_entry *tabptr;
    acquire(&arp_lock);
    for(i = 0; i < ARP_ENTRIES; i++){
        tabptr = &p_arp_table[i];
        if(!ipaddr_cmp(tabptr->ipaddr, 0)){
            if(ipaddr_cmp(ipaddr, tabptr->ipaddr)){
                ethaddr_copy(&tabptr->ethaddr.addr, &ethaddr->addr);
                tabptr->time = arptime;
                release(&arp_lock);
                return;
            }
        }
    }
    /*如果没有找到对应的arp entry，则创建一个*/
    for(i = 0; i < ARP_ENTRIES; i++){
        tabptr = &p_arp_table[i];
        if(ipaddr_cmp(tabptr->ipaddr, 0))
            break;
    }

    /*如果entry满了，则找到一个最老的entry，然后丢弃，填入新的entry*/
    if(i == ARP_ENTRIES){
        int c = 0;
        int tmpage = 0;
        for(i = 0; i < ARP_ENTRIES; i++){
            tabptr = &p_arp_table[i];
            if( (arptime - tabptr->time) > tmpage){
                tmpage = arptime - tabptr->time;
                c = i;
            }
        }
        i = c;
        tabptr = &p_arp_table[i];
    }

    ipaddr_copy(tabptr->ipaddr, ipaddr);
    ethaddr_copy(&tabptr->ethaddr.addr, &ethaddr->addr);
    tabptr->time = arptime;
    release(&arp_lock);
}

/*构建一个arp包*/
struct mbuf *arp_packet_build(uint16 op, struct eth_addr *dmac, ipaddr_t dip)
{
    struct mbuf *m;
    struct arp *arphdr;

    m = mbufalloc(MBUF_DEFAULT_HEADROOM);
    if (!m){
        pr_err("%s %d: failed\n", __func__, __LINE__);
        return NULL;
    }
    // generic part of ARP header
    arphdr = mbufputhdr(m, *arphdr); 
    arphdr->hrd = htons(ARP_HRD_ETHER); // 硬件地址类型，硬件地址不止以太网一种，是以太网类型时，值为1。
    arphdr->pro = htons(ETHTYPE_IP); // 表示要映射的协议地址的类型，要对IPv4地址进行映射，此值为0x0800。
    arphdr->hln = ETHADDR_LEN; //表示硬件地址长度和，MAC地址占6字节
    arphdr->pln = sizeof(ipaddr_t); // 表示协议地址长度，IP地址占4字节
    //操作类型字段，值为1，表示进行ARP请求；值为2，表示进行ARP应答；值为3，表示进行RARP请求；值为4，表示进行RARP应答。
    arphdr->op = htons(op);

    // ethernet + IP part of ARP header
    // 是发送端ARP请求或应答的硬件地址，这里是以太网地址
    ethaddr_copy(&arphdr->sha, &(ip_app_get_local_mac()->addr));
    //发送ARP请求或应答的IP地址。
    arphdr->sip = htonl(ip_app_get_local_ip());
    //目的端的硬件地址
    ethaddr_copy(&arphdr->tha, &dmac->addr);
    //目的端IP地址
    arphdr->tip = htonl(dip);

    return m;
}

int arp_packet_handle(struct arp *arphdr)
{
    ipaddr_t tip, sip;
    if (!arphdr)
        goto done;

    // validate the ARP header
    if (ntohs(arphdr->hrd) != ARP_HRD_ETHER ||
        ntohs(arphdr->pro) != ETHTYPE_IP ||
        arphdr->hln != ETHADDR_LEN ||
        arphdr->pln != sizeof(uint32))
    {
        goto done;
    }

    // only requests are supported so far
    // check if our IP was solicited
    
    tip = ntohl(arphdr->tip); // target IP address
    sip = ntohl(arphdr->sip); //源ip地址

    //neighbor update
    if(ntohs(arphdr->op) == ARP_OP_REQUEST){
        neighbor_add_or_update(sip, (struct neighbor_addr*)&arphdr->sha);
    }

    if (ntohs(arphdr->op) != ARP_OP_REQUEST || (!ipaddr_cmp(tip, ip_app_get_local_ip())))
        goto done;

    arp_update(tip, &arphdr->sha);
    return 0;
done:
    return -1;
}

