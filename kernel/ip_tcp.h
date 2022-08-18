#ifndef __IP_TCP_H__
#define __IP_TCP_H__
#include "types.h"

struct tcp {
    uint16 srcport; //源端口，标识哪个应用程序发送
    uint16 destport; //	目的端口，标识哪个应用程序接收。
    uint32 seqno; //序号字段。TCP链接中传输的数据流中每个字节都编上一个序号。序号字段的值指的是本报文段所发送的数据的第一个字节的序号。
    uint32 ackno; //确认号，是期望收到对方的下一个报文段的数据的第1个字节的序号，即上次已成功接收到的数据字节序号加1。只有ACK标识为1，此字段有效。
    /*
    数据偏移，即首部长度，指出TCP报文段的数据起始处距离TCP报文段的起始处有多远，
    以32比特（4字节）为计算单位。最多有60字节的首部，若无选项字段，正常为20字节。
    */
    uint8 dataoffset:4;
    uint8 reserved:6; //保留，必须填0。
    uint8 urg:1; //紧急指针有效标识。它告诉系统此报文段中有紧急数据，应尽快传送（相当于高优先级的数据）。
    uint8 ack:1; //确认序号有效标识。只有当ACK=1时确认号字段才有效。当ACK=0时，确认号无效。
    uint8 psh:1;//标识接收方应该尽快将这个报文段交给应用层。接收到PSH = 1的TCP报文段，应尽快的交付接收应用进程，而不再等待整个缓存都填满了后再向上交付。
    uint8 rst:1;//重建连接标识。当RST=1时，表明TCP连接中出现严重错误（如由于主机崩溃或其他原因），必须释放连接，然后再重新建立连接。
    uint8 syn:1;//同步序号标识，用来发起一个连接。SYN=1表示这是一个连接请求或连接接受请求。
    uint8 fin:1;//发端完成发送任务标识。用来释放一个连接。FIN=1表明此报文段的发送端的数据已经发送完毕，并要求释放连接。

    uint16 windows;//窗口：TCP的流量控制，窗口起始于确认序号字段指明的值，这个值是接收端正期望接收的字节数。窗口最大为65535字节。
    uint16 checksum;//校验字段，包括TCP首部和TCP数据，是一个强制性的字段，一定是由发端计算和存储，并由收端进行验证。在计算检验和时，要在TCP报文段的前面加上12字节的伪首部。
    uint16 urgent_pointer; //校验字段，包括TCP首部和TCP数据，是一个强制性的字段，一定是由发端计算和存储，并由收端进行验证。在计算检验和时，要在TCP报文段的前面加上12字节的伪首部。
    uint8 options[4];
}__attribute__((packed));


/* Structures and definitions. */
#define TCP_FIN 0x01
#define TCP_SYN 0x02
#define TCP_RST 0x04
#define TCP_PSH 0x08
#define TCP_ACK 0x10
#define TCP_URG 0x20
#define TCP_CTL 0x3f

#define TCP_OPT_END     0   /* End of TCP options list */
#define TCP_OPT_NOOP    1   /* "No-operation" TCP option */
#define TCP_OPT_MSS     2   /* Maximum segment size TCP option */

#define TCP_OPT_MSS_LEN 4   /* Length of TCP MSS option. */


/* The TCP states used in the uip_conn->tcpstateflags. */
#define TCP_CLOSED      0
#define TCP_SYN_RCVD    1
#define TCP_SYN_SENT    2
#define TCP_ESTABLISHED 3
#define TCP_FIN_WAIT_1  4
#define TCP_FIN_WAIT_2  5
#define TCP_CLOSING     6
#define TCP_TIME_WAIT   7
#define TCP_LAST_ACK    8
#define TCP_TS_MASK     15
  
#define TCP_STOPPED      16

#endif
