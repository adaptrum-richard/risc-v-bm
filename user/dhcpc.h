#ifndef __DHCPC_H__
#define __DHCPC_H__
#include "kernel/types.h"
#include "kernel/net.h"

struct dhcp {
    uint8 op;
    uint8 htype;
    uint8 hlen;
    uint8 hops;
    uint32 xid;
    ipaddr_t ciaddr;
    ipaddr_t yiaddr;
    ipaddr_t siaddr;
    ipaddr_t giaddr;
    uint8 chaddr[16];
    uint8 sname[64];
    uint8 file[128];
    uint8 options[312];
};

struct dhcpc_state {
    int fd;
    char state;
    uint8 *mac_addr;
    int mac_len;
    uint8 serverid[4];
    uint8 lease_time[2];
    ipaddr_t ipaddr;
    ipaddr_t netmask;
    ipaddr_t default_router;
};

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

#endif
