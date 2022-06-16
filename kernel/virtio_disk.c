#include "types.h"
#include "riscv.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "fs.h"
#include "virtio.h"
#include "printk.h"
#include "virtio_disk.h"
#include "buf.h"
#include "proc.h"
#include "sched.h"
#include "string.h"
#include "wait.h"
#include "page.h"

// the address of virtio mmio register r.
#define R(r) ((volatile uint32 *)(VIRTIO0 + (r)))

DECLARE_WAIT_QUEUE_HEAD(virtio_disk_handle_queue);

DECLARE_WAIT_QUEUE_HEAD(virtio_disk_free_buf_queue);
static int free_buf_condition = 0;


static struct disk {
  // the virtio driver and device mostly communicate through a set of
  // structures in RAM. pages[] allocates that memory. pages[] is a
  // global (instead of calls to kalloc()) because it must consist of
  // two contiguous pages of page-aligned physical memory.
  char pages[2*PGSIZE];

  // pages[] is divided into three regions (descriptors, avail, and
  // used), as explained in Section 2.6 of the virtio specification
  // for the legacy interface.
  // https://docs.oasis-open.org/virtio/virtio/v1.1/virtio-v1.1.pdf
  
  // the first region of pages[] is a set (not a ring) of DMA
  // descriptors, with which the driver tells the device where to read
  // and write individual disk operations. there are NUM descriptors.
  // most commands consist of a "chain" (a linked list) of a couple of
  // these descriptors.
  // points into pages[].
  struct virtq_desc *desc;

  // next is a ring in which the driver writes descriptor numbers
  // that the driver would like the device to process.  it only
  // includes the head descriptor of each chain. the ring has
  // NUM elements.
  // points into pages[].
  struct virtq_avail *avail;

  // finally a ring in which the device writes descriptor numbers that
  // the device has finished processing (just the head of each chain).
  // there are NUM used ring entries.
  // points into pages[].
  struct virtq_used *used;

  // our own book-keeping.
  char free[NUM];  // is a descriptor free?
  uint16 used_idx; // we've looked this far in used[2..NUM].

  // track info about in-flight operations,
  // for use when completion interrupt arrives.
  // indexed by first descriptor index of chain.
  struct {
    struct buf *b;
    char status;
  } info[NUM];

  // disk command headers.
  // one-for-one with descriptors, for convenience.
  struct virtio_blk_req ops[NUM];
  
  struct spinlock vdisk_lock;
  
} __attribute__ ((aligned (PGSIZE))) disk;

void virtio_disk_init(void)
{
    uint32 status = 0;

    initlock(&disk.vdisk_lock, "virtio_disk");

    if (*R(VIRTIO_MMIO_MAGIC_VALUE) != 0x74726976 ||
        *R(VIRTIO_MMIO_VERSION) != 1 ||
        *R(VIRTIO_MMIO_DEVICE_ID) != 2 ||
        *R(VIRTIO_MMIO_VENDOR_ID) != 0x554d4551)
    {
        panic("could not find virtio disk");
    }

    status |= VIRTIO_CONFIG_S_ACKNOWLEDGE;
    *R(VIRTIO_MMIO_STATUS) = status;

    status |= VIRTIO_CONFIG_S_DRIVER;
    *R(VIRTIO_MMIO_STATUS) = status;

    // negotiate features
    uint64 features = *R(VIRTIO_MMIO_DEVICE_FEATURES);
    features &= ~(1 << VIRTIO_BLK_F_RO);
    features &= ~(1 << VIRTIO_BLK_F_SCSI);
    features &= ~(1 << VIRTIO_BLK_F_CONFIG_WCE);
    features &= ~(1 << VIRTIO_BLK_F_MQ);
    features &= ~(1 << VIRTIO_F_ANY_LAYOUT);
    features &= ~(1 << VIRTIO_RING_F_EVENT_IDX);
    features &= ~(1 << VIRTIO_RING_F_INDIRECT_DESC);
    *R(VIRTIO_MMIO_DRIVER_FEATURES) = features;

    // tell device that feature negotiation is complete.
    status |= VIRTIO_CONFIG_S_FEATURES_OK;
    *R(VIRTIO_MMIO_STATUS) = status;

    // tell device we're completely ready.
    status |= VIRTIO_CONFIG_S_DRIVER_OK;
    *R(VIRTIO_MMIO_STATUS) = status;

    *R(VIRTIO_MMIO_GUEST_PAGE_SIZE) = PGSIZE;

    // initialize queue 0.
    *R(VIRTIO_MMIO_QUEUE_SEL) = 0;
    uint32 max = *R(VIRTIO_MMIO_QUEUE_NUM_MAX);
    if (max == 0)
        panic("virtio disk has no queue 0");
    if (max < NUM)
        panic("virtio disk max queue too short");
    /*设置virtio-disk queue的数量，这里NUM为8*/
    *R(VIRTIO_MMIO_QUEUE_NUM) = NUM;
    memset(disk.pages, 0, sizeof(disk.pages));
    *R(VIRTIO_MMIO_QUEUE_PFN) = ((uint64)disk.pages) >> PGSHIFT;

    /*desc是表示向virtio-disk发送请求时，描述操作系统中的内存位置、请求的长度、请求的类型，
    这里一共支持8个desc，相当于对queue的描述*/
    disk.desc = (struct virtq_desc *) disk.pages;
    /*avail用来记录发送到virtio-disk的数据*/
    disk.avail = (struct virtq_avail *)(disk.pages + NUM*sizeof(struct virtq_desc));
    /*used用来记录virtio-disk发送到driver的数据，会产生中断通知系统*/
    disk.used = (struct virtq_used *) (disk.pages + PGSIZE);

        // all NUM descriptors start out unused.
    for(int i = 0; i < NUM; i++)
        disk.free[i] = 1; //free用来记录哪些disc queue没有使用
}

static void free_desc(int i)
{
    if(i >= NUM)
        panic("free_desc 1");
    if(disk.free[i])
        panic("free_desc 2");
    disk.desc[i].addr = 0;
    disk.desc[i].len = 0;
    disk.desc[i].flags = 0;
    disk.desc[i].next = 0;
    disk.free[i] = 1;

    smp_store_release(&free_buf_condition, 1);
    wake_up(&virtio_disk_free_buf_queue);
}

// free a chain of descriptors.
static void free_chain(int i)
{
    while(1){
        int flag = disk.desc[i].flags;
        int nxt = disk.desc[i].next;
        free_desc(i);
        if(flag & VRING_DESC_F_NEXT)
            i = nxt;
        else
            break;
    }
}

static int alloc_desc()
{
    for(int i = 0; i < NUM; i++){
        if(disk.free[i]){
            disk.free[i] = 0;
            return i;
        }
    }
    return -1;
}

static int alloc3_desc(int *idx)
{
    for(int i = 0; i < 3; i++){
        idx[i] = alloc_desc();
        if(idx[i] < 0){
            /*无法一次性分配3个desc，释放掉已经分配的desc*/
            for(int j = 0; j < i; j++){
                free_desc(idx[j]);
            }
            return -1;
        }
    }
    return 0;
}

void virtio_disk_rw(struct buf *b, int write)
{
    //每个扇区512字节，每个块1024字节
    unsigned long flags;
    uint64 sector = b->blockno * (BSIZE/512);
    int idx[3];

    spin_lock_irqsave(&disk.vdisk_lock, flags);
    while(1){
        if(alloc3_desc(idx) == 0){
            WRITE_ONCE(free_buf_condition, 0);
            break;
        }
        spin_unlock_irqrestore(&disk.vdisk_lock, flags);
        wait_event_interruptible(virtio_disk_free_buf_queue, READ_ONCE(free_buf_condition) == 1);
        spin_lock_irqsave(&disk.vdisk_lock, flags);
    }
    
    struct virtio_blk_req *buf0 = &disk.ops[idx[0]];
    if(write)
        buf0->type = VIRTIO_BLK_T_OUT;//写virio-disk
    else
        buf0->type = VIRTIO_BLK_T_IN;//driver读virio-disk
    
    buf0->reserved = 0;
    buf0->sector = sector;//请求的扇区

    /*第一个描述符：virtio-disk发送的请求的类型(读或写)，操作的扇区,和描述符的类型*/
    disk.desc[idx[0]].addr = (uint64)buf0;
    disk.desc[idx[0]].len = sizeof(struct virtio_blk_req);
    disk.desc[idx[0]].flags = VRING_DESC_F_NEXT;
    disk.desc[idx[0]].next = idx[1];

    /*第二个描述符：读取/写入的数据(操作系统中的内存和长度)*/
    disk.desc[idx[1]].addr = (uint64)b->data;
    disk.desc[idx[1]].len = BSIZE;
    if(write)
        disk.desc[idx[1]].flags = 0;
    else
        disk.desc[idx[1]].flags = VRING_DESC_F_WRITE;
    disk.desc[idx[1]].flags |= VRING_DESC_F_NEXT;
    disk.desc[idx[1]].next = idx[2];

    /*第三个描述符：virtio-disk操作完成后将状态写入到给定的地址中*/
    disk.info[idx[0]].status = 0xff; //设备将此标记设置为0，表示操作成功
    disk.desc[idx[2]].addr = (uint64)&disk.info[idx[0]].status;
    disk.desc[idx[2]].len = 1;
    disk.desc[idx[2]].flags = VRING_DESC_F_WRITE;
    disk.desc[idx[2]].next = 0;

    /*在virtio-disk中断函数中会使用b->disk*/
    b->disk = 1;
    disk.info[idx[0]].b = b;

    /*告诉virtio-disk现在有可用的描述符*/
    disk.avail->ring[disk.avail->idx % NUM] = idx[0];
    __sync_synchronize();

    //告诉virtio-disk下一个有效的的ring entry
    disk.avail->idx += 1;

    /*使用第0个队列，并通知virtio-disk队列你要操作了。*/
    *R(VIRTIO_MMIO_QUEUE_NOTIFY) = 0;

    //等待virtio-disk处理完成，中断函数会将b->disk设置为0
    //while(b->disk == 1){
    spin_unlock_irqrestore(&disk.vdisk_lock, flags);
    wait_event(virtio_disk_handle_queue, READ_ONCE(b->disk) == 0);
    spin_lock_irqsave(&disk.vdisk_lock, flags);
    //}

    disk.info[idx[0]].b = 0;

    /*free_chain中的wake函数将唤醒等待free数组的进程*/
    free_chain(idx[0]);
    spin_unlock_irqrestore(&disk.vdisk_lock, flags);
}

void virtio_disk_intr()
{
    acquire(&disk.vdisk_lock);
    /*
     *这可能与设备向“已用”环写入新条目的竞争，在这种情况下，
     *我们可能会在这个中断中处理新的完成条目，而在下一个中断中没有任何事情要做，这是无害的。
     */
    *R(VIRTIO_MMIO_INTERRUPT_ACK) = *R(VIRTIO_MMIO_INTERRUPT_STATUS) & 0x3;
    __sync_synchronize();
    while(disk.used_idx != disk.used->idx){
        __sync_synchronize();
        int id = disk.used->ring[disk.used_idx % NUM].id;

        if(disk.info[id].status != 0)
            panic("virtio_disk_intr status");

        struct buf *b = disk.info[id].b;
        //b->disk = 0;   // disk is done with buf
        smp_store_release(&b->disk, 0);
        wake_up(&virtio_disk_handle_queue);

        disk.used_idx += 1;
    }

    release(&disk.vdisk_lock);    
}


void disk_read_block(uchar *block, int block_number)
{
    struct buf buf = {0};
    buf.blockno = block_number;
    virtio_disk_rw(&buf, 0);
    memcpy(block, buf.data, BSIZE);
}

void disk_write_block(uchar *block, int block_number)
{
    struct buf buf = {0};
    memcpy(buf.data, block, BSIZE);
    buf.blockno = block_number;
    virtio_disk_rw(&buf, 1);
}