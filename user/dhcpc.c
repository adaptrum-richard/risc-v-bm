#include "user.h"
#include "kernel/types.h"
#include "kernel/net.h"
#include "printf.h"
#include "string.h"
#include "dhcpc.h"

static const uint8 xid[4] = {0xad, 0xde, 0x12, 0x23};
static const uint8 magic_cookie[4] = {99, 130, 83, 99};

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

static void create_dhcp_msg(dhcp_msg_t *m)
{
    /*1：客户端请求报文, 2：服务器响应报文*/
    m->op = DHCP_REQUEST;
    /*表示硬件地址的类型。对于以太网，该类型的值为1*/
    m->htype = DHCP_HTYPE_ETHERNET;
    /*表示硬件地址的长度，单位是字节。对于以太网，该值为6。*/
    m->hlen = dhcpc_s.mac_len;
    /*跳数。客户端设置为0，也能被一个代理服务器设置。*/
    m->hops = 0;
    /*事务ID，由客户端选择的一个随机数，被服务器和客户端用来
    在它们之间交流请求和响应，客户端用它对请求和应答进行匹配。
    该ID由客户端设置并由服务器返回，为32位整数。*/
    memcpy(m->xid, xid, sizeof(m->xid));
    /*由客户端填充，表示从客户端开始获得IP地址或IP地址续借后所使用了的秒数。*/
    m->secs = 0;
    /*此字段在BOOTP中保留未用，在DHCP中表示标志字段。
        只有标志字段的最高位才有意义，其余的位均被置为0。
        最左边的字段被解释为广播响应标志位，内容如下所示：
        0：客户端请求服务器以单播形式发送响应报文
        1：客户端请求服务器以广播形式发送响应报文
    */
    m->flags = htons(BOOTP_BROADCAST);
    /*客户端的IP地址。只有客户端是Bound、Renew、Rebinding状态，并且能响应ARP请求时，才能被填充。*/
    memset(m->ciaddr, 0, sizeof(m->ciaddr));
    /*"你自己的"或客户端的IP地址*/
    memset(m->yiaddr, 0, sizeof(m->yiaddr));
    /*表明DHCP协议流程的下一个阶段要使用的服务器的IP地址。*/
    memset(m->siaddr, 0, sizeof(m->siaddr));
    /*该字段表示第一个DHCP中继的IP地址（注意：不是地址池中定义的网关）。当客户端发出DHCP请求时，
        如果服务器和客户端不在同一个网络中，那么第一个DHCP中继在转发这个DHCP请求报文时会把自己的IP地址填入此字段。
        服务器会根据此字段来判断出网段地址，从而选择为用户分配地址的地址池。服务器还会根据此地址将响应报文发送给
        此DHCP中继，再由DHCP中继将此报文转发给客户端。
        若在到达DHCP服务器前经过了不止一个DHCP中继，那么第一个DHCP中继后的中继不会改变此字段，只是把Hops的数目加1。
    */
    memset(m->giaddr, 0, sizeof(m->giaddr));
    /*
    该字段表示客户端的MAC地址，此字段与前面的“Hardware Type”和“Hardware Length”保持一致。当客户端发出DHCP请求时，
    将自己的硬件地址填入此字段。对于以太网，当“Hardware Type”和“Hardware Length”分别为“1”和“6”时，此字段必须填入6字节的以太网MAC地址。
    */
    memcpy(m->chaddr, dhcpc_s.mac_addr, dhcpc_s.mac_len);
    /*
        该字段表示客户端获取配置信息的服务器名字。此字段由DHCP Server填写，是可选的。如果填写，必须是一个以0结尾的字符串。
    */
    memset(m->sname, 0, sizeof(m->sname));
    /*
        	该字段表示客户端的启动配置文件名。此字段由DHCP Server填写，是可选的，如果填写，必须是一个以0结尾的字符串。
    */
    memset(m->file, 0, sizeof(m->file));

    /*该字段表示DHCP的选项字段，至少为312字节，格式为"代码+长度+数据"。DHCP通过此字段包含了服务器分配给终端的配置信息，
    如网关IP地址，DNS服务器的IP地址，客户端可以使用IP地址的有效租期等信息。*/
    memcpy(m->options, magic_cookie, sizeof(magic_cookie));
}

static void send_discover(void)
{
    uint8 *end;
    dhcp_msg_t m;
    create_msg(&m);
    end = add_msg_type((uint8 *)&m.options[4], DHCPDISCOVER);
    end = add_req_options(end);
    end = add_end(end);
}

void main(void)
{

    exit(0);
}
