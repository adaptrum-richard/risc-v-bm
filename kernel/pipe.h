#ifndef __PIPE_H__
#define __PIPE_H__
#include "types.h"
#include "spinlock.h"
#include "wait.h"
/*
原理参考：
https://blog.csdn.net/qq_42914528/article/details/82023408
*/
#define PIPESIZE (1<<12)
struct pipe {
    struct spinlock lock;
    struct wait_queue_head wait_head;
    char *data;
    uint nread;    // number of bytes read
    uint nwrite;   // number of bytes written
    int readopen;  // read fd is still open
    int writeopen; // write fd is still open
};

#endif
