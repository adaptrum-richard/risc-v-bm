//
// network system calls.
//

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "spinlock.h"
#include "proc.h"
#include "fs.h"
#include "sleeplock.h"
#include "file.h"
#include "net.h"
#include "slab.h"
#include "sysnet.h"
#include "vm.h"
#include "wait.h"
#include "sysfile.h"
#include "printk.h"
#include "ip_app.h"

static struct spinlock lock;
static struct sock *sockets;

void sockinit(void)
{
    initlock(&lock, "socktbl");
}

int sockalloc(struct file **f, uint32 raddr, uint16 lport, uint16 rport)
{
    struct sock *si, *pos;

    si = 0;
    *f = 0;
    if ((*f = filealloc()) == 0)
        goto bad;
    if ((si = (struct sock *)kmalloc(sizeof(struct sock))) == 0)
        goto bad;

    // initialize objects
    si->raddr = raddr;
    si->lport = lport;
    si->rport = rport;
    initlock(&si->lock, "sock");
    mbufq_init(&si->rxq);
    (*f)->type = FD_SOCK;
    (*f)->readable = 1;
    (*f)->writable = 1;
    (*f)->sock = si;
    // add to list of sockets
    acquire(&lock);
    pos = sockets;
    while (pos){
        if (pos->raddr == raddr && pos->lport == lport &&
            pos->rport == rport){
            release(&lock);
            goto bad;
        }
        pos = pos->next;
    }
    si->next = sockets;
    sockets = si;
    release(&lock);
    return 0;

bad:
    if (si)
        kfree((char *)si);
    if (*f)
        fileclose(*f);
    return -1;
}

void sockclose(struct sock *si)
{
    struct sock **pos;
    struct mbuf *m;

    // remove from list of sockets
    acquire(&lock);
    pos = &sockets;
    while (*pos){
        if (*pos == si){
            *pos = si->next;
            break;
        }
        pos = &(*pos)->next;
    }
    release(&lock);

    // free any pending mbufs
    while (!mbufq_empty(&si->rxq)){
        m = mbufq_pophead(&si->rxq);
        mbuffree(m);
    }

    kfree((char *)si);
}

int sockread(struct sock *si, uint64 addr, int n)
{

    struct mbuf *m;
    int len;

    if(mbufq_empty(&si->rxq))
        if(ip_app_wq_timeout_wait_condion((unsigned long)si, 2*HZ) == 0)
            return -1;

    acquire(&si->lock);
    m = mbufq_pophead(&si->rxq);
    release(&si->lock);

    len = m->len;
    if (len > n)
        len = n;

    if (space_data_copy_out(addr, m->head, len) == -1){
        mbuffree(m);
        return -1;
    }
    mbuffree(m);
    return len;
}

int sockwrite(struct sock *si, uint64 addr, int n)
{
    struct mbuf *m;

    m = mbufalloc(MBUF_DEFAULT_HEADROOM);
    if (!m)
        return -1;
    
    if (spcae_data_copy_in(mbufput(m, n), addr, n) == -1){
        mbuffree(m);
        return -1;
    }

    net_tx_udp(m, si->raddr, si->lport, si->rport);
    return n;
}

// called by protocol handler layer to deliver UDP packets
void sockrecvudp(struct mbuf *m, uint32 raddr, uint16 lport, uint16 rport)
{
    //
    // Find the socket that handles this mbuf and deliver it, waking
    // any sleeping reader. Free the mbuf if there are no sockets
    // registered to handle it.
    //
    struct sock *si;

    acquire(&lock);
    si = sockets;
    while (si){
        if (si->raddr == raddr && si->lport == lport && si->rport == rport)
            goto found;
        si = si->next;
    }
    release(&lock);
    mbuffree(m);
    return;

found:
    acquire(&si->lock);
    mbufq_pushtail(&si->rxq, m);
    release(&si->lock);
    release(&lock);
    ip_app_wq_wakeup_process((unsigned long)si);
}

int __sys_connect(uint32 raddr, uint16 lport, uint16 rport)
{
    struct file *f;
    int fd;

    if(sockalloc(&f, raddr, lport, rport) < 0)
        return -1;
    if((fd = fdalloc(f)) < 0){
        fileclose(f);
        return -1;
    }
    return fd;
}