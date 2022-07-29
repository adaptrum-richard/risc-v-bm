#ifndef __SYSNET_H__
#define __SYSNET_H__
#include "net.h"
#include "types.h"
#include "spinlock.h"
#include "wait.h"
#include "file.h"

struct sock
{
    struct sock *next;    // the next socket in the list
    uint32 raddr;         // the remote IPv4 address
    uint16 lport;         // the local UDP port number
    uint16 rport;         // the remote UDP port number
    struct spinlock lock; // protects the rxq
    struct mbufq rxq;     // a queue of packets waiting to be received
    wait_queue_head_t wait_head;
};

void sockinit(void);
int sockalloc(struct file **, uint32, uint16, uint16);
void sockclose(struct sock *);
int sockread(struct sock *, uint64, int);
int sockwrite(struct sock *, uint64, int);
void sockrecvudp(struct mbuf *, uint32, uint16, uint16);
int __sys_connect(uint32 raddr, uint16 lport, uint16 rport);
#endif
