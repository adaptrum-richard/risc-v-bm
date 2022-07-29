#include "pci.h"
#include "printk.h"


void pci_init(void)
{
    unsigned long e1000_regs = VIRT_E1000_ADDR;
    unsigned int *ecam = (unsigned int *)VIRT_PCIE_ADDR;

    for(int dev = 0; dev < 32; dev++){
        int bus = 0;
        int func = 0;
        int offset = 0;
        /*PCI address: 
	    |31 enable bit|30:24 Reserved|23:16 Bus num|15:11 Dev num|10:8 func num|7:2 off|1:0 0|.
        */
        unsigned int off = (bus << 16) | (dev << 11) | (func << 8) | (offset);
        volatile unsigned int *base = ecam + off;

        /*PCI address space header:
	     Byte Off   |   3   |   2   |   1   |   0   |
	              0h|   Device ID   |   Vendor ID   |
        */
        unsigned int id = base[0];
        // 100e:8086 is  e1000
        printk("dev id : 0x%x\n", id);
        if(id == 0x100e8086){
            /*
                command and status register.
                bit 0 : I/O access enable
                bit 1 : memory access enable
                bit 2 : enable mastering
            */
            base[1] = 7;
            __sync_synchronize();
            for(int i = 0; i < 6; i++){
                unsigned int old = base[4+i];
                /*
                 Byte Off              |   3   |   2    |   1     |   0    |
	             16b/4b = 4        10h |           Base Address 0          |
	                      5        14h |           Base Address 1          |
	                      6        18h |           Base Address 2          |
	                      7    1ch~24h |          .... 3, 4, 5             |
                */
                base[4+i] = 0xffffffff;
                __sync_synchronize();

                base[4+i] = old;
            }
            base[4] = e1000_regs;//通过此地址来访问e1000
        }
    }
}
