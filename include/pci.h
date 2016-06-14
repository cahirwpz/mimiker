#ifndef _PCI_H_
#define _PCI_H_

#include <common.h>

typedef struct {
  uint16_t id;
  const char *name;
} pci_device_id;

typedef struct {
  uint16_t id;
  const char *name;
  const pci_device_id *devices;
} pci_vendor_id;

extern const pci_vendor_id pci_vendor_list[];
extern const char *pci_class_code[];

#define PCI_BAR_MEMORY        0
#define PCI_BAR_IO            1
#define PCI_BAR_64BIT         4
#define PCI_BAR_PREFETCHABLE  8

#define PCI_BAR_IO_MASK       3
#define PCI_BAR_MEMORY_MASK  15

typedef struct {
  pm_addr_t addr;
  size_t size;
} pci_bar_t;

typedef struct {
  struct {
    uint8_t bus;
    uint8_t device;
    uint8_t function;
  } addr;

  uint16_t device_id;
  uint16_t vendor_id;
  uint8_t  class_code;
  uint8_t  pin, irq;

  uint8_t   nbars;
  pci_bar_t *bar;
} pci_device_t;

void pci_init();

#endif /* _PCI_H_ */
