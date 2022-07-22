#include "virtio_net.h"
#include "types.h"
#include "virtio.h"
#include "printk.h"
#include "memlayout.h"
#include "skb.h"

#define RN(r) ((volatile uint32 *)(VIRTIO1 + (r)))

static struct net {

  char pages[2*PGSIZE];
  struct virtq_desc *desc;
  struct virtq_avail *avail;
  struct virtq_used *used;
  char free[NUM];
  uint16 used_idx;
  struct {
    struct skb_buf *b;
    char status;
  } info[NUM];
} __attribute__ ((aligned (PGSIZE))) net;

void virtio_net_init(void)
{
    uint32 status = 0;

    if (*RN(VIRTIO_MMIO_MAGIC_VALUE) != 0x74726976 ||
        *RN(VIRTIO_MMIO_VERSION) != 1 ||
        *RN(VIRTIO_MMIO_DEVICE_ID) != 1 ||
        *RN(VIRTIO_MMIO_VENDOR_ID) != 0x554d4551){
        panic("could not find virtio net");
    }
    // negotiate features: default 0x39bf8064
    /*
        VIRTIO_NET_F_CTRL_GUEST_OFFLOADS 2
        VIRTIO_NET_F_MAC 5
        VIRTIO_NET_F_GSO 6
        VIRTIO_NET_F_MRG_RXBUF 15
        VIRTIO_NET_F_STATUS	16	            virtio_net_config.status available
        VIRTIO_NET_F_CTRL_VQ	17	        Control channel available
        VIRTIO_NET_F_CTRL_RX	18	        Control channel RX mode support
        VIRTIO_NET_F_CTRL_VLAN	19	        Control channel VLAN filtering
        VIRTIO_NET_F_CTRL_RX_EXTRA 20	    Extra RX mode control support 
        VIRTIO_NET_F_GUEST_ANNOUNCE 21	    Guest can announce device on the network
        VIRTIO_NET_F_CTRL_MAC_ADDR 23	    Set MAC address

        Do we get callbacks when the ring is completely used, even if we've suppressed them?
        VIRTIO_F_NOTIFY_ON_EMPTY	24

        Can the device handle any descriptor layout?
        VIRTIO_F_ANY_LAYOUT		27
        VIRTIO_TRANSPORT_F_START	28
    */
    uint64 features = *RN(VIRTIO_MMIO_DEVICE_FEATURES);
    printk("network device default features = 0x%lx\n", features);
    features &= ~(1 << VIRTIO_NET_F_GSO);
    features &= ~(1 << VIRTIO_NET_F_MRG_RXBUF);
    features &= ~(1 << VIRTIO_NET_F_GUEST_UFO);
    features &= ~(1 << VIRTIO_NET_F_CTRL_VLAN);
    features &= ~(1 <<  VIRTIO_NET_F_MQ);
    *RN(VIRTIO_MMIO_DRIVER_FEATURES) = features;
    printk("network device new features = 0x%lx\n", features);


    // tell device that feature negotiation is complete.
    status |= VIRTIO_CONFIG_S_FEATURES_OK;
    *RN(VIRTIO_MMIO_STATUS) = status;

    // tell device we're completely ready.
    status |= VIRTIO_CONFIG_S_DRIVER_OK;
    *RN(VIRTIO_MMIO_STATUS) = status;

    *RN(VIRTIO_MMIO_GUEST_PAGE_SIZE) = PGSIZE;
        // initialize queue 0.
    *RN(VIRTIO_MMIO_QUEUE_SEL) = 0;
    uint32 max = *RN(VIRTIO_MMIO_QUEUE_NUM_MAX);
    if (max == 0)
        panic("virtio net has no queue 0");
    if (max < NUM)
        panic("virtio net max queue too short");
    printk("virtio net queue max num: %u\n", max);

    /*设置virtio-net queue的数量，这里NUM为8*/
    *RN(VIRTIO_MMIO_QUEUE_NUM) = NUM;
    memset(net.pages, 0, sizeof(net.pages));
    *RN(VIRTIO_MMIO_QUEUE_PFN) = ((uint64)net.pages) >> PGSHIFT;

    /*desc是表示向virtio-net发送请求时，描述操作系统中的内存位置、请求的长度、请求的类型，
    这里一共支持8个desc，相当于对queue的描述*/
    net.desc = (struct virtq_desc *) net.pages;
    /*avail用来记录发送到virtio-net的数据*/
    net.avail = (struct virtq_avail *)(net.pages + NUM*sizeof(struct virtq_desc));
    /*used用来记录virtio-net发送到driver的数据，会产生中断通知系统*/
    net.used = (struct virtq_used *) (net.pages + PGSIZE);

    // all NUM descriptors start out unused.
    for(int i = 0; i < NUM; i++)
        net.free[i] = 1; //free用来记录哪些disc queue没有使用
}


static void free_desc(int i)
{
    if(i >= NUM)
        panic("free_desc 1");
    if(net.free[i])
        panic("free_desc 2");
    net.desc[i].addr = 0;
    net.desc[i].len = 0;
    net.desc[i].flags = 0;
    net.desc[i].next = 0;
    net.free[i] = 1;

    //smp_store_release(&free_buf_condition, 1);
    //wake_up(&virtio_disk_free_buf_queue);
}

// free a chain of descriptors.
static void free_chain(int i)
{
    while(1){
        int flag = net.desc[i].flags;
        int nxt = net.desc[i].next;
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
        if(net.free[i]){
            net.free[i] = 0;
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

    int idx[3];

    //spin_lock_irqsave(&disk.vdisk_lock, flags);
    while(1){
        if(alloc3_desc(idx) == 0){
            //WRITE_ONCE(free_buf_condition, 0);
            break;
        }
        //spin_unlock_irqrestore(&disk.vdisk_lock, flags);
        //wait_event_interruptible(virtio_disk_free_buf_queue, READ_ONCE(free_buf_condition) == 1);
        //spin_lock_irqsave(&disk.vdisk_lock, flags);
    }
    
    struct virtio_blk_req *buf0 = &net.ops[idx[0]];
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

