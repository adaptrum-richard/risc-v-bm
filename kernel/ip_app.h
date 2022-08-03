#ifndef __IP_APP_H__
#define __IP_APP_H__
#include "net.h"

void print_ipaddr(ipaddr_t *ipaddr);
ipaddr_t ip_app_get_local_ip(void);
struct eth_addr *ip_app_get_local_mac(void);
struct eth_addr *ip_app_get_broadcast_mac(void);
ipaddr_t ip_app_get_broadcast_ipaddr();
#endif
