#ifndef _SYS_PCI_DRIVERS_H_
#define _SYS_PCI_DRIVERS_H_

#include <drivers.h>

typedef struct pci_driver {
  char *name;
  driver_t driver;
} pci_driver_t;

#endif /* _SYS_PCI_DRIVERS_H_ */
