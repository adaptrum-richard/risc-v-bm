#ifndef __BUF_H__
#define __BUF_H__

#include "types.h"
#include "fs.h"
#include "sleeplock.h"

struct buf {
  int valid;   // has data been read from disk?
  int disk;    // does disk "own" buf?
  uint dev;
  uint blockno;
  struct sleeplock lock;
  uint refcnt;
  struct buf *prev; // LRU cache list
  struct buf *next;
  uchar data[BSIZE];
};

#endif
