#include "types.h"
#include "log.h"
#include "param.h"
#include "spinlock.h"
#include "fs.h"
#include "printk.h"
#include "virtio_disk.h"
#include "buf.h"
#include "string.h"
#include "bio.h"
#include "sched.h"

/*
WAL机制
WAL即 Write Ahead Log，WAL的主要意思是说在将元数据的变更操作写入磁盘之前，先预先写入到一个log文件中
1.内存的读写速度比随机读写磁盘高出几个数量级。
2.磁盘顺序读写效率堪比内存
ref：https://zhuanlan.zhihu.com/p/228335203
*/

static void recover_from_log(void);
static void install_trans(int recovering);
static void commit(void);

struct log log;

void initlog(int dev, struct superblock *sb)
{
    if(sizeof(struct logheader) >= BSIZE)
        panic("initlog : too big logheader\n");
    
    initlock(&log.lock, "log");
    log.start = sb->logstart;
    log.size = sb->nlog;
    log.dev = dev;
    recover_from_log();
}

static void write_log(void)
{
    for(int i = 0; i < log.lh.n; i++){
        /*+1表示跳过logheader的block，logheader后面才是存放备份的数据block*/
        struct buf *to = bread(log.dev, log.start + i + 1);
        /*从cache的block中读取数据，注意不是从disk中读取*/
        struct buf *from = bread(log.dev, log.lh.block[i]);
        memmove(to->data, from->data, BSIZE);
        bwrite(to);

        brelse(from);
        brelse(to);
    }
}


/*线程执行文件写操作时，先将数据写入到cache block中，而不是直接写入到设备*/
void log_write(struct buf *b)
{
    int i;
    acquire(&log.lock);
    if(log.lh.n >= LOGSIZE || log.lh.n >= log.size - 1)
        panic("too big a transaction\n");

    /*只有当没有线程在执行文件操作的时候才能写log*/
    if(log.outstanding < 1)
        panic("log_write outside of trans\n");
    
    for(i = 0; i < log.lh.n; i++)
        if(log.lh.block[i] == b->blockno)
            break;
    log.lh.block[i] = b->blockno;
    if( i == log.lh.n ){
        /*如果走到这里说明，需要在log的插槽中分配一个*/
        bpin(b);
        log.lh.n++;
    }
    release(&log.lock);
}

static void write_head(void)
{
    struct buf *b = bread(log.dev, log.start);
    struct logheader *hb = (struct logheader *)(b->data);

    hb->n = log.lh.n;
    for(int i = 0; i < log.lh.n; i++){
        hb->block[i] = log.lh.block[i];
    }
    bwrite(b);
    brelse(b);
}

static void read_head(void)
{
    struct buf *b = bread(log.dev, log.start);
    struct logheader *hb = (struct logheader *)(b->data);

    hb->n = log.lh.n;
    for(int i = 0; i < log.lh.n; i++){
        log.lh.block[i] = hb->block[i];
    }
    brelse(b);    
}

void log_begin_op(void)
{
    acquire(&log.lock);
    while(1){
        if(log.committing) {
            /*如果有线程正在提交log，则等待提交完成后，线程才能进入新的文件操作*/
            release(&log.lock);
            wait((uint64)&log);
            acquire(&log.lock);
        } else if(log.lh.n + (log.outstanding + 1)*MAXOPBLOCKS > LOGSIZE) {
            /*如果同时操作文件的线程或次数超过了log空间能承受的范围，则等待commit log*/
            release(&log.lock);
            wait((uint64)&log);
            acquire(&log.lock);
        } else {
            /*线程开始执行文件操作前需要将log.outstanding加1*/
            log.outstanding++;
            release(&log.lock);
            break;
        }
    }
}

void log_end_op(void)
{
    acquire(&log.lock);
    /*线程执行文件操作后需要将log.outstanding减1*/
    log.outstanding--;
    if(log.committing){
        /*在commit log的时候不会有线程在执行文件操作*/
        panic("log.committing\n");
    }

    if(log.outstanding == 0){
        /*当前没有线程对文件进行操作，则可以commit log*/
        log.committing = 1;
        commit();
        log.committing = 0;
        release(&log.lock);
        wake((uint64)&log);
    }else {
        release(&log.lock);
        /*唤醒在log_begin_to等待的线程*/
        wake((uint64)&log);
    }
}

static void commit(void)
{
    if(log.lh.n > 0){
        write_log();
        write_head();
        install_trans(0);
        log.lh.n = 0;
        write_head();//擦除日志
    }
}

static void install_trans(int recovering)
{
    for(int i = 0; i < log.lh.n; i++){
        //读log的block
        struct buf *bsrc = bread(log.dev, log.start + i + 1);
        //读数据的block
        struct buf *bdst = bread(log.dev, log.lh.block[i]);
        memmove(bdst->data, bsrc->data, BSIZE);
        bwrite(bdst);
        if(recovering == 0)
            bunpin(bdst);
        brelse(bsrc);
        brelse(bdst);
    }
}

static void recover_from_log(void)
{
    read_head();
    install_trans(1);
    log.lh.n = 0;
    write_head();
}
