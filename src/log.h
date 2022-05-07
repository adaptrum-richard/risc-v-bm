#ifndef __LOG_H__
#define __LOG_H__
#include "param.h"
#include "spinlock.h"
#include "buf.h"
#include "fs.h"

struct logheader {
    int n;
    int block[LOGSIZE];
};

struct log {
    struct spinlock lock;
    int start;
    int size;
    int outstanding;
    int committing;
    int dev;
    struct logheader lh;
};

void log_begin_op(void);
void log_end_op(void);
void log_write(struct buf *b);
void initlog(int dev, struct superblock *sb);
#endif
