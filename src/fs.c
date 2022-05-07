#include "fs.h"
#include "buf.h"
#include "bio.h"
#include "string.h"
#include "log.h"
#include "printk.h"
#include "sched.h"

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


