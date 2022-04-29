#ifndef __BIO_H__
#define __BIO_H__

#include "types.h"
#include "spinlock.h"
#include "param.h"
#include "buf.h"

struct bcache{
  struct spinlock lock;
  struct buf buf[NBUF];
  struct buf head;
};

void binit(void);
struct buf *bread(uint dev, uint blockno);
void bwrite(struct buf *b);
void brelse(struct buf *b);
void bpin(struct buf *b);
void bunpin(struct buf *b);

#endif
