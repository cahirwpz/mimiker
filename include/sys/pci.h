#ifndef _SYS_PCI_H_
#define _SYS_PCI_H_

#include <sys/cdefs.h>
#include <sys/queue.h>
#include <sys/device.h>
#include <sys/bus.h>
#include <sys/rman.h>

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

/* Please read http://wiki.osdev.org/PCI */

#define PCI_BAR_MEMORY 0
#define PCI_BAR_IO 1
#define PCI_BAR_64BIT 4
#define PCI_BAR_PREFETCHABLE 8

#define PCI_BAR_IO_MASK 3
#define PCI_BAR_MEMORY_MASK 15

#define PCIR_DEVICEID 0x00
#define PCIR_VENDORID 0x02
#define PCIR_STATUS 0x04
#define PCIR_COMMAND 0x06
#define PCIM_CMD_PORTEN 0x0001
#define PCIM_CMD_MEMEN 0x0002
#define PCIR_CLASSCODE 0x08
#define PCIR_IRQPIN 0x3e
#define PCIR_IRQLINE 0x3f
#define PCIR_BAR(i) (0x10 + (i)*4)

typedef struct pci_addr {
  uint8_t bus;
  uint8_t device;
  uint8_t function;
} pci_addr_t;

/* PCI configuration space can be accessed in various ways depending on PCI bus
 * controller and platform architecture. For instance classic PC architecture
 * uses `in` and `out` IA-32 instructions. On MIPS Malta one has to use two
 * memory mapped registers (address and data). */
typedef uint32_t (*pci_read_config_t)(device_t *device, unsigned reg,
                                      unsigned size);
typedef void (*pci_write_config_t)(device_t *device, unsigned reg,
                                   unsigned size, uint32_t value);

typedef struct pci_bus_methods {
  pci_read_config_t read_config;
  pci_write_config_t write_config;
} pci_bus_methods_t;

typedef struct pci_bus_driver {
  driver_t driver;
  bus_methods_t bus;
  pci_bus_methods_t pci_bus;
} pci_bus_driver_t;

typedef struct pci_bar {
  device_t *owner; /* pci device owner of this bar */
  size_t size;     /* identified size of this bar */
  int rid;         /* BAR number in [0,5] */
  unsigned type;   /* RT_IOPORTS or RT_MEMORY */
  unsigned flags;  /* nothing or RF_PREFETACHBLE */
} pci_bar_t;

typedef struct pci_device {
  pci_addr_t addr;

  uint16_t device_id;
  uint16_t vendor_id;
  uint8_t class_code;
  uint8_t pin, irq;

  unsigned nbars;
  pci_bar_t bar[6]; /* identified BARs */
} pci_device_t;

#define PCI_DRIVER(dev) ((pci_bus_driver_t *)((dev)->parent->driver))

static inline uint32_t pci_read_config(device_t *device, unsigned reg,
                                       unsigned size) {
  return PCI_DRIVER(device)->pci_bus.read_config(device, reg, size);
}

static inline void pci_write_config(device_t *device, unsigned reg,
                                    unsigned size, uint32_t value) {
  PCI_DRIVER(device)->pci_bus.write_config(device, reg, size, value);
}

static inline uint32_t pci_adjust_config(device_t *device, unsigned reg,
                                         unsigned size, uint32_t value) {
  pci_write_config(device, reg, size, value);
  return pci_read_config(device, reg, size);
}

void pci_bus_enumerate(device_t *pcib);
void pci_bus_assign_space(device_t *pcib);
void pci_bus_dump(device_t *pcib);

static inline pci_device_t *pci_device_of(device_t *device) {
  return (device->bus == DEV_BUS_PCI) ? device->instance : NULL;
}

#define DEVICE_DRIVER_GEN_PCI_PROBE(vendor__id, device__id, device_name)    \
  static int dev_generic_probe_ ## device_name(device_t *dev) {             \
    pci_device_t *pcid = pci_device_of(dev);                                \
    if (pcid->vendor_id != (vendor__id) || pcid->device_id != (device__id)) \
      return 0;                                                             \
  /* if (!(pcid->bar[0].flags & RF_PREFETCHABLE)) */                        \
  /*   return 0; */                                                         \
  return 1;                                                                 \
  }   

#endif /* !_SYS_PCI_H_ */
