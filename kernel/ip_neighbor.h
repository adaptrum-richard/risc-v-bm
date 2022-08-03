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
void neighbor_add(ipaddr_t ipaddr, struct neighbor_addr *addr);
void neighbor_periodic(void);
void ip_neighbor_init(void);

#endif
