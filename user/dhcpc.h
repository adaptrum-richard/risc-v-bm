#ifndef __DHCPC_H__
#define __DHCPC_H__
#include "kernel/types.h"
#include "kernel/net.h"
#include "string.h"

typedef struct dhcp_msg{
    uint8 op;
    uint8 htype;
    uint8 hlen;
    uint8 hops;
    uint32 xid;
    uint16 secs;
    uint16 flags;
    ipaddr_t ciaddr;
    ipaddr_t yiaddr;
    ipaddr_t siaddr;
    ipaddr_t giaddr;
    uint8 chaddr[16];
    uint8 sname[64];
    uint8 file[128];
    uint8 options[312];
}dhcp_msg_t  __attribute__((packed));

typedef struct dhcpc_state {
    int fd;
    char state;
    uint8 *mac_addr;
    int mac_len;
    uint8 serverid[4];
    uint8 lease_time[2];
    ipaddr_t ipaddr;
    ipaddr_t netmask;
    ipaddr_t default_router;
}dhcpc_state_t;

#define STATE_INITIAL         0
#define STATE_SENDING         1
#define STATE_OFFER_RECEIVED  2
#define STATE_CONFIG_RECEIVED 3


#define BOOTP_BROADCAST 0x8000

#define DHCP_REQUEST        1
#define DHCP_REPLY          2
#define DHCP_HTYPE_ETHERNET 1
#define DHCP_HLEN_ETHERNET  6
#define DHCP_MSG_LEN      236

#define DHCPC_SERVER_PORT  67
#define DHCPC_CLIENT_PORT  68

#define DHCPDISCOVER  1
#define DHCPOFFER     2
#define DHCPREQUEST   3
#define DHCPDECLINE   4
#define DHCPACK       5
#define DHCPNAK       6
#define DHCPRELEASE   7

#define DHCP_OPTION_SUBNET_MASK   1
#define DHCP_OPTION_ROUTER        3
#define DHCP_OPTION_DNS_SERVER    6
#define DHCP_OPTION_REQ_IPADDR   50
#define DHCP_OPTION_LEASE_TIME   51
#define DHCP_OPTION_MSG_TYPE     53
#define DHCP_OPTION_SERVER_ID    54
#define DHCP_OPTION_REQ_LIST     55
#define DHCP_OPTION_END         255

static inline unsigned char *
add_msg_type(unsigned char *optptr, unsigned char type)
{
    *optptr++ = DHCP_OPTION_MSG_TYPE;
    *optptr++ = 1;
    *optptr++ = type;
    return optptr;
}

static inline unsigned char *
add_server_id(unsigned char *optptr, unsigned char *serverid)
{
    *optptr++ = DHCP_OPTION_SERVER_ID;
    *optptr++ = 4;
    memcpy(optptr, serverid, 4);
    return optptr + 4;
}

static inline unsigned char *
add_req_ipaddr(unsigned char *optptr, unsigned int *ipaddr)
{
    *optptr++ = DHCP_OPTION_REQ_IPADDR;
    *optptr++ = 4;
    memcpy(optptr, ipaddr, 4);
    return optptr + 4;
}

static inline unsigned char *
add_req_options(unsigned char *optptr)
{
    *optptr++ = DHCP_OPTION_REQ_LIST;
    *optptr++ = 3;
    *optptr++ = DHCP_OPTION_SUBNET_MASK;
    *optptr++ = DHCP_OPTION_ROUTER;
    *optptr++ = DHCP_OPTION_DNS_SERVER;
    return optptr;
}

static unsigned char *
add_end(unsigned char *optptr)
{
    *optptr++ = DHCP_OPTION_END;
    return optptr;
}

#endif
