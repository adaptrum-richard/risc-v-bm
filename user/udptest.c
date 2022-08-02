#include "user.h"
#include "printf.h"
#include "string.h"
#include "kernel/types.h"

static void send_udp(unsigned short sport, unsigned short dport)
{
    int fd;
    char *obuf = "this a risc-v-bm message!";
    uint32 dst;
    dst = (10 << 24) | (0 << 16) | (2 << 8) | (2 << 0);
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

void main(void)
{
    unsigned short dport = NET_TESTS_PORT;
    printf("testing udp sport:2000, dport:%d\n", dport);
    send_udp(2000, dport);
}
