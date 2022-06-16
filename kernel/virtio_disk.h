#ifndef __VIRTIO_DISK_H__
#define __VIRTIO_DISK_H__

#include "types.h"
#include "buf.h"

void virtio_disk_init(void);
void virtio_disk_intr();
void virtio_disk_rw(struct buf *b, int write);
void disk_read_block(uchar *block, int block_number);
void disk_write_block(uchar *block, int block_number);

#endif
