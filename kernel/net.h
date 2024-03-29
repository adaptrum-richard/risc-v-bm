#ifndef __NET_H__
#define __NET_H__
#include "types.h"

#define MBUF_SIZE 2048
#define MBUF_DEFAULT_HEADROOM 128

struct mbuf
{
    struct mbuf *next;   // the next mbuf in the chain
    char *head;          // the current start position of the buffer
    unsigned int len;    // the length of the buffer
    char buf[MBUF_SIZE]; // the backing store
};

char *mbufpull(struct mbuf *m, unsigned int len);
char *mbufpush(struct mbuf *m, unsigned int len);
char *mbufput(struct mbuf *m, unsigned int len);
char *mbuftrim(struct mbuf *m, unsigned int len);

// The above functions manipulate the size and position of the buffer:
//            <- push            <- trim
//             -> pull            -> put
// [-headroom-][------buffer------][-tailroom-]
// |----------------MBUF_SIZE-----------------|
//
// These marcos automatically typecast and determine the size of header structs.
// In most situations you should use these instead of the raw ops above.
#define mbufpullhdr(mbuf, hdr) (typeof(hdr) *)mbufpull(mbuf, sizeof(hdr))
#define mbufpushhdr(mbuf, hdr) (typeof(hdr) *)mbufpush(mbuf, sizeof(hdr))
#define mbufputhdr(mbuf, hdr) (typeof(hdr) *)mbufput(mbuf, sizeof(hdr))
#define mbuftrimhdr(mbuf, hdr) (typeof(hdr) *)mbuftrim(mbuf, sizeof(hdr))

struct mbuf *mbufalloc(unsigned int headroom);
void mbuffree(struct mbuf *m);

struct mbufq
{
    struct mbuf *head; // the first element in the queue
    struct mbuf *tail; // the last element in the queue
};

void mbufq_pushtail(struct mbufq *q, struct mbuf *m);
struct mbuf *mbufq_pophead(struct mbufq *q);
int mbufq_empty(struct mbufq *q);
void mbufq_init(struct mbufq *q);

// endianness support
static inline uint16 bswaps(uint16 val)
{
    return (((val & 0x00ffU) << 8) |
            ((val & 0xff00U) >> 8));
}

static inline uint32 bswapl(uint32 val)
{
    return (((val & 0x000000ffUL) << 24) |
            ((val & 0x0000ff00UL) << 8) |
            ((val & 0x00ff0000UL) >> 8) |
            ((val & 0xff000000UL) >> 24));
}

// Use these macros to convert network bytes to the native byte order.
// Note that Risc-V uses little endian while network order is big endian.
#define ntohs bswaps
#define ntohl bswapl
#define htons bswaps
#define htonl bswapl

// useful networking headers
#define ETHADDR_LEN 6
// an Ethernet packet header (start of the packet).
struct eth
{
    uint8 dhost[ETHADDR_LEN];
    uint8 shost[ETHADDR_LEN];
    uint16 type;
} __attribute__((packed));

//以太网的地址
struct eth_addr {
    uint8 addr[ETHADDR_LEN];
};

typedef uint32 ipaddr_t;
#define ipaddr_cmp(addr1, addr2) (addr1 == addr2)
#define ipaddr_copy(addr1, addr2) (addr1 = addr2)
#define ethaddr_copy(ethaddr1, ethaddr2) (memcpy(ethaddr1, ethaddr2, ETHADDR_LEN))
#define ethaddr_cmp(ethaddr1, ethaddr2) (!memcmp(ethaddr1, ethaddr2, ETHADDR_LEN))
#define ETHTYPE_IP 0x0800  // Internet protocol
#define ETHTYPE_ARP 0x0806 // Address resolution protocol

// an IP packet header (comes after an Ethernet header).
struct ip
{
    /*4：表示为IPV4；首部长度，如果不带Option字段，则为20，最长为60，该值限制了记录路由选项。以4字节为一个单位。*/
    uint8 ip_vhl;  // version << 4 | header length >> 2
    uint8 ip_tos;  // type of service
    uint16 ip_len; // total length
    uint16 ip_id;  // identification
    uint16 ip_off; // fragment offset field
    uint8 ip_ttl;  // time to live
    uint8 ip_p;    // protocol
    uint16 ip_sum; // checksum
    uint32 ip_src, ip_dst;
};

#define IPPROTO_ICMP 1 // Control message protocol
#define IPPROTO_TCP 6  // Transmission control protocol
#define IPPROTO_UDP 17 // User datagram protocol

#define MAKE_IP_ADDR(a, b, c, d)             \
    (((uint32)a << 24) | ((uint32)b << 16) | \
     ((uint32)c << 8) | (uint32)d)

// a UDP packet header (comes after an IP header).
struct udp
{
    uint16 sport; // source port
    uint16 dport; // destination port
    uint16 ulen;  // length, including udp header, not including IP header
    uint16 sum;   // checksum
};

// an DNS packet (comes after an UDP header).
struct dns
{
    uint16 id; // request ID

    uint8 rd : 1; // recursion desired
    uint8 tc : 1; // truncated
    uint8 aa : 1; // authoritive
    uint8 opcode : 4;
    uint8 qr : 1;    // query/response
    uint8 rcode : 4; // response code
    uint8 cd : 1;    // checking disabled
    uint8 ad : 1;    // authenticated data
    uint8 z : 1;
    uint8 ra : 1; // recursion available

    uint16 qdcount; // number of question entries
    uint16 ancount; // number of resource records in answer section
    uint16 nscount; // number of NS resource records in authority section
    uint16 arcount; // number of resource records in additional records
} __attribute__((packed));

struct dns_question
{
    uint16 qtype;
    uint16 qclass;
} __attribute__((packed));

#define ARECORD (0x0001)
#define QCLASS (0x0001)

struct dns_data
{
    uint16 type;
    uint16 class;
    uint32 ttl;
    uint16 len;
} __attribute__((packed));

void net_tx_udp(struct mbuf *m, uint32 dip, uint16 sport, uint16 dport);
void net_rx(struct mbuf *m);
void net_init(void);

#endif
