#ifndef __IP_ARP_H__
#define __IP_ARP_H__
#include "net.h"

struct arp_entry {
    ipaddr_t ipaddr;
    struct eth_addr ethaddr;
    uint64 time;
};

// an ARP packet (comes after an Ethernet header).
//参考https://www.cnblogs.com/juankai/p/10315957.html
struct arp
{
    uint16 hrd; // 硬件地址类型，硬件地址不止以太网一种，是以太网类型时，值为1。
    uint16 pro; // 表示要映射的协议地址的类型，要对IPv4地址进行映射，此值为0x0800。
    uint8 hln;  // 表示硬件地址长度和，MAC地址占6字节，
    uint8 pln;  // 表示协议地址长度，IP地址占4字节
    uint16 op;  // 操作类型字段，值为1，表示进行ARP请求；值为2，表示进行ARP应答；值为3，表示进行RARP请求；值为4，表示进行RARP应答。

    struct eth_addr sha; // 是发送端ARP请求或应答的硬件地址，这里是以太网地址
    ipaddr_t sip;            //是发送ARP请求或应答的IP地址。
    struct eth_addr tha; // 是目的端的硬件地址
    ipaddr_t tip;            // 目的端IP地址
} __attribute__((packed));

#define ARP_HRD_ETHER 1 // Ethernet

enum
{
    ARP_OP_REQUEST = 1, // requests hw addr given protocol addr
    ARP_OP_REPLY = 2,   // replies a hw addr given protocol addr
};

#define ARP_ENTRIES 8
#define ARP_MAXAGE_TIME 120

struct mbuf *arp_packet_build(uint16 op, struct eth_addr *dmac, ipaddr_t dip);
int arp_packet_handle(struct arp *arphdr);
void ip_arp_init(void);
int arp_lookup(ipaddr_t ipaddr, struct eth_addr *mac);
#endif
