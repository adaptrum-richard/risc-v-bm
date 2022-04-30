#include "bio.h"
#include "buf.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "virtio_disk.h"
#include "printk.h"

struct bcache bcache;

/*初始化bio，将buf连接成双向环形链表*/
void binit(void)
{
    struct buf *b;
    initlock(&bcache.lock, "bcache");

    bcache.head.prev = &bcache.head;
    bcache.head.next = &bcache.head;

    for(b = bcache.buf; b < bcache.buf + NBUF; b++){
        b->next = bcache.head.next;
        b->prev = &bcache.head;
        bcache.head.next->prev = b;
        bcache.head.next = b;
        initsleeplock(&b->lock, "buffer");
    }
}

static struct buf *bget(uint dev, uint blockno)
{
    struct buf *b;
    acquire(&bcache.lock);

    for(b = bcache.head.next; b != &bcache.head; b = b->next){
        if(b->dev == dev && b->blockno == blockno){
            b->refcnt++;
            release(&bcache.lock);
            /*acquiresleep 保证一个buf只有一个线程在操作*/
            acquiresleep(&b->lock);
            return b;
        }
    }
    
    //block没有被缓存
    for(b = bcache.head.prev; b != &bcache.head; b = b->prev){
        if(b->refcnt == 0){
            b->dev = dev;
            b->blockno = blockno;
            b->valid = 0;
            b->refcnt = 1;
            release(&bcache.lock);
            acquiresleep(&b->lock);
            return b;
        }
    }
    panic("bget: no buffers");
    return 0;
}

struct buf *bread(uint dev, uint blockno)
{
    struct buf *b;
    b = bget(dev, blockno);
    if(!b->valid){
        virtio_disk_rw(b, 0);
        b->valid = 1;
    }
    return b;
}

void bwrite(struct buf *b)
{
    if(holdingsleep(&b->lock) == 0)
        panic("bwrite");
    virtio_disk_rw(b, 1);
}

void brelse(struct buf *b)
{
    if(holdingsleep(&b->lock) == 0)
        panic("brelse");

    //设置locked为1，pid为0，唤醒等待操作此buf的线程
    releasesleep(&b->lock);

    acquire(&bcache.lock);
    b->refcnt--;
    if(b->refcnt == 0){
        b->next->prev = b->prev;
        b->prev->next = b->next;
        b->next = bcache.head.next;
        b->prev = &bcache.head;
        bcache.head.next->prev = b;
        bcache.head.next = b;
    }
    release(&bcache.lock);
}

void bpin(struct buf *b)
{
    acquire(&bcache.lock);
    b->refcnt++;
    release(&bcache.lock);
}

void bunpin(struct buf *b)
{
    acquire(&bcache.lock);
    b->refcnt--;
    release(&bcache.lock);
}
