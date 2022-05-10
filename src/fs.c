#include "fs.h"
#include "buf.h"
#include "bio.h"
#include "string.h"
#include "log.h"
#include "printk.h"
#include "sched.h"
#include "file.h"

struct superblock sb;

void itrunc(struct inode *ip);
void iupdate(struct inode *ip);

void print_superblock(void)
{
    printk("magic\t\t\t0x%x\n", sb.magic);
    printk("size\t\t\t%u blocks\n", sb.size);
    printk("nblocks\t\t\t%u\n", sb.nblocks);
    printk("ninodes\t\t\t%u\n", sb.ninodes);
    printk("nlog\t\t\t%u\n", sb.nlog);
    printk("logstart block\t\t%u\n", sb.logstart);
    printk("inodestart block\t%u\n", sb.inodestart);
    printk("bmapstart block\t\t%u\n", sb.bmapstart);
}

static void readsb(int dev, struct superblock *sb)
{
    struct buf *b;

    /*读取第一个block,即supperblock*/
    b = bread(dev, 1);
    memmove(sb, b->data, sizeof(*sb));
    brelse(b);
}

void fsinit(int dev)
{
    readsb(dev, &sb);
    //sleep(10);
    print_superblock();
    if(sb.magic != FSMAGIC)
        panic("invalid file system");
    initlog(dev, &sb);
}

static void bzero(int dev, int bno)
{
    struct buf *b;
    b = bread(dev, bno);
    memset(b->data, 0, BSIZE);
    log_write(b);
    brelse(b);
}

/*分配一个block，返回block number*/
static int balloc(int dev)
{
    int b, bi, m;
    struct buf *bp = 0;

    for(b = 0; b < sb.size; b += BPB){
        /*1.读取b对应的block，然后读取此bitmap block*/
        bp = bread(dev, BBLOCK(b, sb));
        /*2.遍历每个bitmap中的位*/
        for(bi = 0; bi < BPB && ((b*BPB) + bi < sb.size); bi++){
            m = 1 << (bi % 8);
            if((bp->data[bi/8] & m) == 0){
                bp->data[bi/8] |= m;
                log_write(bp);
                brelse(bp);
                bzero(dev,b + bi);
                return b + bi;
            }
        }
        brelse(bp);
    }
    panic("balloc: out of blocks\n");
    return 0;
}

static void bfree(int dev, int bn)
{
    struct buf *bp;
    int bi, m;
    bp = bread(dev, BBLOCK(bn, sb));
    bi = bn % BPB;
    m = 1 << (bi % 8);
    if((bp->data[bi/8] & m) == 0){
        panic("freeing free block\n");
    }
    bp->data[bi/8] &= ~m;
    log_write(bp);
    brelse(bp);
}

struct {
  struct spinlock lock;
  struct inode inode[NINODE];
} itable;

void iinit(void)
{
    int i = 0;
    initlock(&itable.lock, "itable");
    for(i = 0; i < NINODE; i++){
        initsleeplock(&itable.inode[i].lock, "inode");
    }
}

static struct inode *iget(int dev, int inum);

struct inode *ialloc(int dev, short type)
{
    int inum;
    struct buf *bp;
    struct dinode *dip;
    for(inum = 1; inum < sb.ninodes; inum++){
        /*从磁盘中读取一个dinode*/
        bp = bread(dev, IBLOCK(inum, sb));
        dip = (struct dinode *)bp->data + inum % IPB;
        if(dip->type == 0){
            /*a free inode*/
            memset(dip, 0, sizeof(*dip));
            dip->type = type;
            log_write(bp);
            brelse(bp);
            /*将从磁盘中读取的dinode，在内存建立一个映射inode*/
            return iget(dev, inum);
        }
        brelse(bp);
    }
    panic("ialloc: no inodes");
    return 0;
}

void ilock(struct inode *ip)
{
    struct buf *bp;
    struct dinode *dip;
    if(ip == 0 || ip->ref < 1)
        panic("ilock");

    acquiresleep(&ip->lock);
    
    if(ip->valid == 0){
        bp = bread(ip->dev, IBLOCK(ip->inum, sb));
        dip = (struct dinode *)bp->data + ip->inum % IPB;
        ip->type = dip->type;
        ip->major = dip->minor;
        ip->nlink = dip->nlink;
        ip->size = dip->size;
        memmove(ip->addrs, dip->addrs, sizeof(ip->addrs));
        brelse(bp);
        ip->valid = 1;
        if(ip->type == 0)
            panic("ilock: no type\n");
    }
}

void iunlock(struct inode *ip)
{
    if(ip == 0 || !holdingsleep(&ip->lock) || ip->ref < 1)
        panic("iunlock");
    
    releasesleep(&ip->lock);
}

/*从itable中分配一个inode slot*/
static struct inode *iget(int dev, int inum)
{
    struct inode *ip, *empty;
    acquire(&itable.lock);
    empty = 0;
    for(ip = &itable.inode[0]; ip < &itable.inode[NINODE]; ip++){
        if(ip->ref > 0 && ip->dev == dev && ip->inum == inum){
            ip->ref++;
            release(&itable.lock);
            return ip;
        }
        /*记录一个空的slot*/
        if(empty == 0 && ip->ref == 0)
            empty = ip;
    }
    if(empty == 0)
        panic("iget: no inodes\n");

    ip = empty;
    ip->dev = dev;
    ip->inum = inum;
    ip->ref = 1;
    ip->valid = 0;
    release(&itable.lock);
    return ip;
}

void iput(struct inode *ip)
{
    acquire(&itable.lock);

    if(ip->ref == 1 && ip->valid && ip->nlink == 0){
        /*当ip->ref等于1时，表示没有其他进程在持有此inode，所以也不会有
          其他进程持有inode->lock锁。这里不会出现死锁*/
        acquiresleep(&ip->lock);

        release(&itable.lock);
        /*acquiresleep(&ip->lock);不放置在这里，是因为在两个锁之间没有锁保护的话，ip->ref可能会被修改*/
        itrunc(ip);
        ip->type = 0;
        iupdate(ip);
        ip->valid = 0;
        
        releasesleep(&ip->lock);

        acquire(&itable.lock);
    }
    ip->ref--;
    release(&itable.lock);
}

/*内存中的inode更新到磁盘的dnode中*/
void iupdate(struct inode *ip)
{
    struct buf *bp;
    struct dinode *dip;
    bp = bread(ip->dev, IBLOCK(ip->inum, sb));
    dip = (struct dinode *)bp->data + ip->inum % IPB;
    dip->type = ip->type;
    dip->major = ip->major;
    dip->minor = ip->minor;
    dip->nlink = ip->nlink;
    dip->size = ip->size;
    memmove(dip->addrs, ip->addrs, sizeof(ip->addrs));
    log_write(bp);
    brelse(bp);
}

/*释放inode中对应的block，并设置文件大小为0*/
void itrunc(struct inode *ip)
{
    int i, j;
    struct buf *bp;
    uint *a;
    for(i = 0; i < NDIRECT; i++){
        if(ip->addrs[i]){
            bfree(ip->dev, ip->addrs[i]);//在bmap上设置为0
            ip->addrs[i] = 0;
        }
    }

    if(ip->addrs[NDIRECT]){
        bp = bread(ip->dev, ip->addrs[NDIRECT]);
        a = (uint *)bp->data;
        for(j = 0; j < NINDIRECT; j++){
            if(a[j])
                bfree(ip->dev, a[j]);
        }
        brelse(bp);
        bfree(ip->dev, ip->addrs[NDIRECT]);
        ip->addrs[NDIRECT] = 0;
    }
    ip->size = 0;
    iupdate(ip);
}
