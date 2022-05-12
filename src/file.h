#ifndef __FILE_H__
#define __FILE_H__
#include "types.h"
#include "fs.h"
#include "sleeplock.h"

#define major(dev) ((dev) >> 16 & 0xFFFF)
#define minor(dev) ((dev)&0xFFFF)
#define mkdev(m, n) ((uint)((m) << 16 | (n)))

// in-memory copy of an inode
struct inode
{
    uint dev;              // Device number
    uint inum;             // Inode number
    int ref;               // Reference count
    struct sleeplock lock; // protects everything below here
    int valid;             // inode has been read from disk?

    short type; // copy of disk inode
    short major;
    short minor;
    short nlink;
    uint size;
    /*
     *NDIRECT = 12, 12个直接块，第13个是一级间接块,
     *默认大小为(12+256)*BISZE = 268KB
     */
    uint addrs[NDIRECT + 1];
};

// map major device number to device functions.
struct devsw
{
    int (*read)(uint64, int);
    int (*write)(uint64, int);
};

struct file {
    enum { FD_NONE, FD_INODE, FD_DEVICE } type;
    int ref; // reference count
    char readable;
    char writable;
    struct inode *ip;  // FD_INODE and FD_DEVICE
    uint off;          // FD_INODE
    short major;       // FD_DEVICE
};

extern struct devsw devsw[];

#define CONSOLE 1

void fileinit(void);
struct file *filealloc(void);
void fileclose(struct file *f);
int fileread(struct file *f, uint64 addr, int n);
int filewrite(struct file *f, uint64 addr, int n);
#endif
