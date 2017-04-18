#ifndef _SYS_PCI_DRIVERS_H_
#define _SYS_PCI_DRIVERS_H_

#include <pci.h>
#include <drivers.h>
#include <common.h>

typedef struct pci_devid {
  uint16_t vendor;
  uint16_t device;
} pci_devid_t;

typedef int pci_probe_func(pci_device_t *dev, pci_devid_t *id);

typedef struct pci_driver {
  char *name;
  pci_devid_t *devids;
  pci_probe_func *probe;
  driver_t driver;
} pci_driver_t;

int pci_driver_register(pci_driver_t *drv);

#endif /* _SYS_PCI_DRIVERS_H_ */
