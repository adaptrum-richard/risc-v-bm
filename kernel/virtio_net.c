#include "virtio_net.h"
#include "types.h"
#include "virtio.h"
#include "printk.h"
#include "memlayout.h"

#define RN(r) ((volatile uint32 *)(VIRTIO1 + (r)))

void virtio_net_init(void)
{
    //uint32 status = 0;

    if (*RN(VIRTIO_MMIO_MAGIC_VALUE) != 0x74726976 ||
        *RN(VIRTIO_MMIO_VERSION) != 1 ||
        *RN(VIRTIO_MMIO_DEVICE_ID) != 1 ||
        *RN(VIRTIO_MMIO_VENDOR_ID) != 0x554d4551){
        panic("could not find virtio net");
    }
    // negotiate features
    uint64 features = *RN(VIRTIO_MMIO_DEVICE_FEATURES);
    printk("features = 0x%lx\n", features);

}