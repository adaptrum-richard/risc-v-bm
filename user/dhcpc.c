#include "user.h"
#include "kernel/types.h"
#include "kernel/net.h"
#include "printf.h"
#include "dhcpc.h"

static dhcpc_state_t dhcpc_s;


static int dhcpc_init(const void *mac_addr, int mac_len)
{
    ipaddr_t addr = MAKE_IP_ADDR(255, 255, 255, 255);
    dhcpc_s.mac_addr = mac_addr;
    dhcpc_s.mac_len = mac_len;
    
    dhcpc_s.state = STATE_INITIAL;
    dhcpc_s.fd = connect(addr, DHCPC_CLIENT_PORT, DHCPC_SERVER_PORT);
    if(dhcpc_s.fd < 0){
        printf("%s %d error\n", __func__, __LINE__);
        return -1;
    }
    return 0;
}

static void send_discover(void)
{
    
}

void main(void)
{

    exit(0);
}
