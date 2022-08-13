#include "net.h"
#include "types.h"
#include "printk.h"
#include "slab.h"
#include "string.h"
#include "e1000.h"
#include "sysnet.h"
#include "ip_arp.h"
#include "ip_app.h"
#include "debug.h"
#include "ip_neighbor.h"

static int net_tx_arp(uint16 op, struct eth_addr *dmac, ipaddr_t dip);
/*
从缓冲区的起点剥离数据，并返回指向该缓冲区的指针。
如果小于请求的完整长度，则返回0
*/
char *mbufpull(struct mbuf *m, unsigned int len)
{
    char *tmp = m->head;
    if (m->len < len)
        return 0;
    m->len -= len;
    m->head += len;
    return tmp;
}

/*
将数据头往前移动len字节，并返回指向该缓冲区的指针。
*/
char *mbufpush(struct mbuf *m, unsigned int len)
{
    m->head -= len;
    if (m->head < m->buf)
        panic("mbufpush");
    m->len += len;
    return m->head;
}

/*
将数据追加到缓冲区的末尾，并返回指向该缓冲区的指针。
数据长度需要累加。
*/
char *mbufput(struct mbuf *m, unsigned int len)
{
    char *tmp = m->head + m->len;
    m->len += len;
    if (m->len > MBUF_SIZE)
        panic("mbufput");
    return tmp;
}

/*
从缓冲区的末端剥离数据并返回指向该缓冲区的指针。
如果小于请求的完整长度返回0。
*/
char *mbuftrim(struct mbuf *m, unsigned int len)
{
    if (len > m->len)
        return 0;
    m->len -= len;
    return m->head + m->len;
}

// Allocates a packet buffer.
struct mbuf *mbufalloc(unsigned int headroom)
{
    struct mbuf *m;

    if (headroom > MBUF_SIZE)
        return NULL;
    m = kmalloc(sizeof(struct mbuf));
    if (m == NULL)
        return NULL;
    m->next = NULL;
    m->head = (char *)m->buf + headroom;
    m->len = 0;
    memset(m->buf, 0, sizeof(m->buf));
    return m;
}

// Frees a packet buffer.
void mbuffree(struct mbuf *m)
{
    kfree(m);
}

// Pushes an mbuf to the end of the queue.
void mbufq_pushtail(struct mbufq *q, struct mbuf *m)
{
    m->next = 0;
    if (!q->head)
    {
        q->head = q->tail = m;
        return;
    }
    q->tail->next = m;
    q->tail = m;
}

// Pops an mbuf from the start of the queue.
struct mbuf *mbufq_pophead(struct mbufq *q)
{
    struct mbuf *head = q->head;
    if (!head)
        return 0;
    q->head = head->next;
    return head;
}

// Returns one (nonzero) if the queue is empty.
int mbufq_empty(struct mbufq *q)
{
    return q->head == NULL;
}

// Intializes a queue of mbufs.
void mbufq_init(struct mbufq *q)
{
    q->head = NULL;
}

static unsigned short in_cksum(const unsigned char *addr, int len)
{
    int nleft = len;
    const unsigned short *w = (const unsigned short *)addr;
    unsigned int sum = 0;
    unsigned short answer = 0;

    /*
     * Our algorithm is simple, using a 32 bit accumulator (sum), we add
     * sequential 16 bit words to it, and at the end, fold back all the
     * carry bits from the top 16 bits into the lower 16 bits.
     */
    while (nleft > 1)
    {
        sum += *w++;
        nleft -= 2;
    }

    /* mop up an odd byte, if necessary */
    if (nleft == 1)
    {
        *(unsigned char *)(&answer) = *(const unsigned char *)w;
        sum += answer;
    }

    /* add back carry outs from top 16 bits to low 16 bits */
    sum = (sum & 0xffff) + (sum >> 16);
    sum += (sum >> 16);
    /* guaranteed now that the lower 16 bits of sum are correct */

    answer = ~sum; /* truncate to 16 bits */
    return answer;
}

// sends an ethernet packet
static void net_tx_eth(struct mbuf *m, uint16 ethtype)
{
    struct eth *ethhdr;
    struct eth_addr mac;
    long ret;
    struct ip *iphdr= (struct ip*)m->head;
    struct arp *arphdr = (struct arp*)m->head;

    ethhdr = mbufpushhdr(m, *ethhdr);

    /*
     *1.源ip地址不为0.0.0.0，
     *2.目的ip地址和源ip地址是同网段，
     *3.目的ip地址不为255.255.255.255
     *4.目的ip地址在arp_tbl中没有对应的mac地址，
     *满足上述4个要求发出arp请求
    */
    if(ethtype == ETHTYPE_IP && !ipaddr_cmp(iphdr->ip_src, MAKE_IP_ADDR(0, 0, 0, 0))
        && !ipaddr_cmp(iphdr->ip_dst, MAKE_IP_ADDR(0xff, 0xff, 0xff, 0xff))
        && !ipaddr_cmp(ip_app_get_local_netmask(), MAKE_IP_ADDR(0, 0, 0, 0))
        && ipaddr_netcmp(ntohl(iphdr->ip_dst), ip_app_get_local_ip(), ip_app_get_local_netmask())){
        if(arp_lookup(ntohl(iphdr->ip_dst), &mac) == 0)
            ethaddr_copy(ethhdr->dhost, mac.addr);
        else{
            net_tx_arp(ARP_OP_REQUEST, ip_app_get_broadcast_mac(), ntohl(iphdr->ip_dst));
            ret = ip_app_wq_timeout_wait_condition((unsigned long)ntohl(iphdr->ip_dst), 2*HZ);
            if(ret == 0){
                printk("%s %d: wait arp reply timeout\n", __func__, __LINE__);
                ethaddr_copy(&ethhdr->dhost, &ip_app_get_broadcast_mac()->addr);
            }else {
                if(arp_lookup(iphdr->ip_dst, &mac) == 0){
                    ethaddr_copy(ethhdr->dhost, mac.addr);
                }
            }
        }
    } else{
        if(ethtype == ETHTYPE_ARP)
            ethaddr_copy(ethhdr->dhost, arphdr->tha.addr);
        else 
            ethaddr_copy(ethhdr->dhost, &ip_app_get_broadcast_mac()->addr);
    }

    
    ethaddr_copy(&ethhdr->shost, &ip_app_get_local_mac()->addr);
    ethhdr->type = htons(ethtype);

    if (e1000_transmit(m))
        mbuffree(m);
}

// sends an IP packet
static void net_tx_ip(struct mbuf *m, uint8 proto, uint32 dip)
{
    struct ip *iphdr;

    // push the IP header
    iphdr = mbufpushhdr(m, *iphdr);
    memset(iphdr, 0, sizeof(*iphdr));
    iphdr->ip_vhl = (4 << 4) | (20 >> 2);
    iphdr->ip_p = proto;
    iphdr->ip_src = htonl(ip_app_get_local_ip());
    iphdr->ip_dst = htonl(dip);
    iphdr->ip_len = htons(m->len);
    iphdr->ip_ttl = 100;
    iphdr->ip_sum = in_cksum((unsigned char *)iphdr, sizeof(*iphdr));

    // now on to the ethernet layer
    net_tx_eth(m, ETHTYPE_IP);
}

// sends a UDP packet
void net_tx_udp(struct mbuf *m, uint32 dip, uint16 sport, uint16 dport)
{
    struct udp *udphdr;

    // put the UDP header
    udphdr = mbufpushhdr(m, *udphdr);
    udphdr->sport = htons(sport);
    udphdr->dport = htons(dport);
    udphdr->ulen = htons(m->len);
    udphdr->sum = 0; // zero means no checksum is provided

    // now on to the IP layer
    net_tx_ip(m, IPPROTO_UDP, dip);
}

// sends an ARP packet
static int net_tx_arp(uint16 op, struct eth_addr *dmac, ipaddr_t dip)
{
    struct mbuf *m;
    m = arp_packet_build(op, dmac, dip);
    if(!m){
        pr_err("%s %d: failed\n", __func__, __LINE__);
        return -1;
    }
    // header is ready, send the packet
    net_tx_eth(m, ETHTYPE_ARP);
    return 0;
}

// receives an ARP packet
static void net_rx_arp(struct mbuf *m)
{
    struct arp *arphdr;
    struct eth_addr smac;
    uint32 sip;
    arphdr = mbufpullhdr(m, *arphdr);
    if (!arphdr)
        goto done;

    if(arp_packet_handle(arphdr) < 0)
        goto done;

    // handle the ARP request
    ethaddr_copy(&smac, &arphdr->sha);// sender's ethernet address
    sip = ntohl(arphdr->sip);                // sender's IP address (qemu's slirp)
    ip_app_wq_wakeup_process((unsigned long)sip);
    net_tx_arp(ARP_OP_REPLY, &smac, sip);
done:
    mbuffree(m);
}

// receives a UDP packet
static void net_rx_udp(struct mbuf *m, uint16 len, struct ip *iphdr)
{
    struct udp *udphdr;
    uint32 sip;
    uint16 sport, dport;

    udphdr = mbufpullhdr(m, *udphdr);
    if (!udphdr)
        goto fail;

    // TODO: validate UDP checksum

    // validate lengths reported in headers
    if (ntohs(udphdr->ulen) != len)
        goto fail;
    len -= sizeof(*udphdr);
    if (len > m->len)
        goto fail;
    // minimum packet size could be larger than the payload
    mbuftrim(m, m->len - len);

    // parse the necessary fields
    sip = ntohl(iphdr->ip_src);
    sport = ntohs(udphdr->sport);
    dport = ntohs(udphdr->dport);
    sockrecvudp(m, sip, dport, sport);
    return;

fail:
    mbuffree(m);
}

// receives an IP packet
static void net_rx_ip(struct mbuf *m)
{
    struct ip *iphdr;
    uint16 len;
    iphdr = mbufpullhdr(m, *iphdr);
    if (!iphdr)
        goto fail;

    // check IP version and header len
    if (iphdr->ip_vhl != ((4 << 4) | (20 >> 2)))
        goto fail;
    // validate IP checksum
    if (in_cksum((unsigned char *)iphdr, sizeof(*iphdr)))
        goto fail;
    // can't support fragmented IP packets
    if (htons(iphdr->ip_off) != 0)
        goto fail;
    // is the packet addressed to us?
    if(htonl(iphdr->ip_dst) == MAKE_IP_ADDR(255,255,255,255))
        goto skip;
    
    if (htonl(iphdr->ip_dst) != ip_app_get_local_ip())
        goto fail;
skip:
    // can only support UDP
    if (iphdr->ip_p != IPPROTO_UDP)
        goto fail;

    len = ntohs(iphdr->ip_len) - sizeof(*iphdr);
    net_rx_udp(m, len, iphdr);
    return;

fail:
    mbuffree(m);
}

// called by e1000 driver's interrupt handler to deliver a packet to the
// networking stack
void net_rx(struct mbuf *m)
{
    struct eth *ethhdr;
    uint16 type;

    ethhdr = mbufpullhdr(m, *ethhdr);
    if (!ethhdr)
    {
        mbuffree(m);
        return;
    }

    type = ntohs(ethhdr->type);
    if (type == ETHTYPE_IP)
        net_rx_ip(m);
    else if (type == ETHTYPE_ARP)
        net_rx_arp(m);
    else
        mbuffree(m);
}

void net_init(void)
{
    ip_neighbor_init();
    ip_arp_init();
}
