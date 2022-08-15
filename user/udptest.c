#include "user.h"
#include "printf.h"
#include "string.h"
#include "kernel/types.h"
#include "ulib.h"
#include "kernel/net.h"
/*
测试方法：在一个终端中运行
$ python3 server.py 26099
另外一个终端运行make run 然后执行udptest
*/


static void send_udp(uint32 dst, unsigned short sport, unsigned short dport)
{
    int fd;
    char *obuf = "this a risc-v-bm message!";

    if ((fd = connect(dst, sport, dport)) < 0)
    {
        fprintf(2, "ping: connect() failed\n");
        exit(1);
    }

    if (write(fd, obuf, strlen(obuf)) < 0)
    {
        fprintf(2, "ping: send() failed\n");
        exit(1);
    }
    char ibuf[128];
    printf("read data\n");
    int cc = read(fd, ibuf, sizeof(ibuf) - 1);
    if (cc < 0)
    {
        fprintf(2, "ping: recv() failed\n");
        exit(1);
    }
    ibuf[cc] = '\0';
    printf("rcv: %s\n", ibuf);
    close(fd);
}
/*
用法：
udptest 192.168.100.2
*/
void main(int argc, char *argv[])
{
    unsigned short dport = NET_TESTS_PORT;
    char str_ipaddr[128] = {0};
    unsigned int dst = 0;
    uint16 sport = 2000;
    if(argc == 2){
        strcpy(str_ipaddr, argv[1]);
        dst = inet_addr(str_ipaddr);
        dst = ntohl(dst);
    }
    if(dst == 0)
        dst = (10 << 24) | (0 << 16) | (2 << 8) | (2 << 0);
    printf("udp dst:%s, sport: %d, dport:%d\n",str_ipaddr, sport, dport);
    send_udp(dst, 2000, dport);
    exit(0);
}
