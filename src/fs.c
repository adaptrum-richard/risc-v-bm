#include "fs.h"
#include "buf.h"
#include "bio.h"
#include "string.h"
#include "log.h"
#include "printk.h"
#include "sched.h"
#include "file.h"
#include "stat.h"

struct superblock sb;

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
        ip->major = dip->major;
        ip->minor = dip->minor;
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

/*通过文件在inode中addr的编号，找到对应的block编号*/
static uint bmap(struct inode *ip, uint bn)
{
    uint addr, *a;
    struct buf *bp;

    if(bn < NDIRECT){
        if((addr = ip->addrs[bn]) == 0){
            ip->addrs[bn] = addr = balloc(ip->dev);
        }
        return addr;
    }
    bn -= NDIRECT;

    if(bn < NINDIRECT){
        if( (addr = ip->addrs[NDIRECT]) == 0){
            ip->addrs[NDIRECT] = addr = balloc(ip->dev);
        }
        bp = bread(ip->dev, addr);
        a = (uint *)bp->data;
        if((addr = a[bn]) == 0){
            a[bn] = addr = balloc(ip->dev);
            log_write(bp);
        }
        brelse(bp);
        return addr;
    }
    panic("bmap: out of range");
    return 0;
}

int readi(struct inode *ip, uint64 dst, uint off, uint n)
{
    uint tot, m;
    struct buf *bp;

    /*off + n < off 防止溢出*/
    if(off > ip->size || off + n < off)
        return 0;
    
    if(off + n > ip->size)
        n = ip->size - off;
    
    for(tot = 0; tot < n ; tot += m, off += m, dst += m){
        bp = bread(ip->dev, bmap(ip, off/BSIZE));
        m = min(n -  tot, BSIZE- off % BSIZE);
        memmove((char *)dst, bp->data + (off % BSIZE), m);
        brelse(bp);
    }
    return tot;
}

int writei(struct inode *ip, uint64 src, uint off, uint n)
{
    uint tot, m;
    struct buf *bp;

    if(off > ip->size || off + n < off)
        return -1;
    
    if(off + n > MAXFILE*BSIZE)
        return -1;

    for(tot = 0; tot < n; tot += m, off += m, src += m){
        bp = bread(ip->dev, bmap(ip, off/BSIZE));
        m = min(n - tot, BSIZE - off % BSIZE);
        memmove(bp->data + (off % BSIZE), (char *)src, m);
        log_write(bp);
        brelse(bp);
    }

    if(off > ip->size)
        ip->size = off;
    
    iupdate(ip);
    return tot;
}

struct inode *idup(struct inode *ip)
{
    acquire(&itable.lock);
    ip->ref++;
    release(&itable.lock);
    return ip;
}

/*
将path中最开始的路径放到name中。需要通过name去查找对应的inode的编号
如：
skipelem("a/bb/c", name) = "bb/c", setting name = "a"
skipelem("///a//bb", name) = "bb", setting name = "a"
skipelem("a", name) = "", setting name = "a"
skipelem("", name) = skipelem("////", name) = 0
*/
static char* skipelem(char *path, char *name)
{
    char *s;
    int len;

    while(*path == '/')
        path++;
    if(*path == 0)
        return 0;
    s = path;
    while(*path != '/' && *path != 0)
        path++;
    len = path - s;
    if(len >= DIRSIZ)
        memmove(name, s, DIRSIZ);
    else {
        memmove(name, s, len);
        name[len] = 0;
    }
    while(*path == '/')
        path++;
    return path;
}

int namecmp(const char *s, const char *t)
{
    return strncmp(s, t, DIRSIZ);
}

struct inode *dirlookup(struct inode *dp, char *name, uint *poff)
{
    uint off, inum;
    struct dirent de;
    if(dp->type != T_DIR){
        panic("dirlookup not DIR\n");
        return 0;
    }

    /*读取目录block中存储的每个dirent结构体，判断目录或文件名是否为我们要查找的*/
    for(off = 0; off < dp->size; off += sizeof(de)){
        if(readi(dp, (uint64)&de, off, sizeof(de)) != sizeof(de)){
            panic("dirlookup read\n");
            return 0;
        }
        if(de.inum == 0)
            continue;
        
        if(namecmp(name, de.name) == 0){
            if(poff)
                *poff = off;
            inum = de.inum;
            /*范围此目录或文件对应的inode*/
            return iget(dp->dev, inum);
        }
    }
    return 0;
}

void iunlockput(struct inode *ip)
{
    iunlock(ip);
    iput(ip);
}

static struct inode *namex(char *path, int nameiparent, char *name)
{
    struct inode *ip, *next;
    if(*path == '/')
        ip = iget(ROOTDEV, ROOTINO);
    else {
        panic("Unimplemented function\n");
        return 0;
    }
    /*
    一层一层遍历path，从path对开始遍历，比如目录为/a/b/c
    则在根目录下的dinode的addr中遍历，查找对应a的inode，
    然后再在a目录下的dinode的addr中遍历，依次类推，最终找到c文件
    对应的dinode
    */
    while((path = skipelem(path, name)) != 0){
        ilock(ip);
        if(ip->type != T_DIR){
            iunlockput(ip);
            return 0;
        }

        if(nameiparent && *path == '\0'){
            iunlock(ip);
            return ip;
        }

        if((next = dirlookup(ip, name, 0)) == 0){
            iunlockput(ip);
            return 0;
        }

        iunlockput(ip);
        ip = next;
    }

    if(nameiparent){
        iput(ip);
        return 0;
    }

    return ip;
}

struct inode *namei(char *path)
{
    char name[DIRSIZ];
    return namex(path, 0, name);
}

struct inode* nameiparent(char *path, char *name)
{
  return namex(path, 1, name);
}

/*写一个新的文件目录(name, inmu)到struct inode *dp中*/
int dirlink(struct inode *dp, char *name, uint inum)
{
    int off;
    struct dirent de;
    struct inode *ip;

    if((ip = dirlookup(dp, name, 0)) != 0){
        iput(ip);
        return -1;
    }

    for(off = 0; off < dp->size; off += sizeof(de)){
        if(readi(dp, (uint64)&de, off, sizeof(de)) !=sizeof(de)){
            panic("dirlink read\n");
            return 0;
        }
        if(de.inum == 0)
            break;
    }

    strncpy(de.name, name, DIRSIZ);
    de.inum = inum;
    if(writei(dp, (uint64)&de, off, sizeof(de)) != sizeof(de)){
        panic("dirlink\n");
    }
    return 0;
}
