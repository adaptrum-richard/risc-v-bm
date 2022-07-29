#ifndef __PCI_H__
#define __PIC_H__

#define VIRT_PCIE_ADDR 0x30000000UL
#define VIRT_E1000_ADDR 0x40000000UL

#define VIRT_PCIE_ADDR_LENGTH 0x10000000
#define VIRT_E1000_ADDR_LENGTH 0x20000
void pci_init(void);
#endif
