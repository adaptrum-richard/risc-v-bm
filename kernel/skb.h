#ifndef __SKB_H__
#define __SKB_H__
#include "types.h"
#define SKBSIZE 2048

struct skb_buf {
  int valid;
  int len;
  uint refcnt;
  uchar data[SKBSIZE];
};

#endif
