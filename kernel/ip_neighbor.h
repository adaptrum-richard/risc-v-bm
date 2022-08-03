#ifndef __IP_NEIGHBOR_H__
#define __IP_NEIGHBOR_H__
#include "net.h"

struct neighbor_addr{
    struct eth_addr addr;
};

struct neighbor_entry
{
    ipaddr_t ipaddr;
    struct neighbor_addr addr;
    uint8 time;
};

#define MAX_NEIGHBOR_TIME 128
#define NEIGHBOR_ENTRIES 8

int neighbor_lookup(ipaddr_t ipaddr, struct neighbor_addr *na);
void neighbor_add_or_update(ipaddr_t ipaddr, struct neighbor_addr *na);
void neighbor_periodic(void);
void ip_neighbor_init(void);


/*
https://blog.csdn.net/bigsheng2/article/details/122776855

neighbour项是在什么时候创建的呢?

这需要从两个方向来分析，发送与接收：

1、对于发送方向来说，当路由器需要转发或者需要自己发送一个数据包时，
会去查找路由表当查找到的路由没有在路由缓存中时，则需要为该路由建立
一个路由缓存并加入到路由缓存链表中，同时会调用arp_bind_neighbour
实现路由缓存与neighbour的绑定(如果没有相应的neighbour项，
则创建neighbour项)。然后再判断neighbour项是否可用，若不可用，则将
数据包存入队列中，并发送arp 请求，在接收到请求后，则将neighbour项
设置为可用，并将数据从队列中取出并发送出去其邻居项的状态转换为
NUD_NONE -> NUD_INCOMPLETE -> NUD_REACHABLE。

2、对于接收方向来说，当主机接收到arp request报文，则认为主机与发送
请求报文之间的链路为通的，则为该发送主机创建一个邻居表项，并将其状态
设置为NUD_STATE，其邻居项的状态转换为
NUD_NONE -> NUD_STALE -> NUD_DELAY -> NUD_PROBE -> NUD_REACHABLE
*/
#endif
