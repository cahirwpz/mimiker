#ifndef _SYS_PCI_H_
#define _SYS_PCI_H_

#include <common.h>
#include <queue.h>
#include <bus.h>

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

#define PCI_IO_SPACE_BASE 0x1000

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
#define PCIR_CLASSCODE 0x08
#define PCIR_IRQPIN 0x3e
#define PCIR_IRQLINE 0x3f
#define PCIR_BAR(i) (0x10 + (i)*4)

typedef struct pci_device pci_device_t;
typedef TAILQ_HEAD(, pci_device) pci_device_list_t;

typedef struct {
  pm_addr_t addr;
  size_t size;
  /* The two fields below are stored for convenience. They seem redundant,
     but as we'll be sorting all bars by their size, keeping a reference
     to where an entry came from is useful. */
  pci_device_t *dev; /* Reference to the parent device. */
  unsigned i;        /* Current BAR number for parent device. */
} pci_bar_t;

typedef struct pci_addr {
  uint8_t bus;
  uint8_t device;
  uint8_t function;
} pci_addr_t;

typedef uint32_t (*pci_read_config_t)(pci_device_t *device, unsigned reg,
                                      unsigned size);
typedef void (*pci_write_config_t)(pci_device_t *device, unsigned reg,
                                   unsigned size, uint32_t value);

typedef struct pci_bus {
  pci_read_config_t read_config;
  pci_write_config_t write_config;
} pci_bus_t;

#define PCI_BUS_DECLARE(name) extern pci_bus_t name[1]

struct pci_device {
  TAILQ_ENTRY(pci_device) link;

  pci_bus_t *bus;
  pci_addr_t addr;

  uint16_t device_id;
  uint16_t vendor_id;
  uint8_t class_code;
  uint8_t pin, irq;

  unsigned nbars;
  pci_bar_t bar[6];
};

typedef struct pci_bus_device {
  pci_bus_t *bus;
  resource_t *mem_space;
  resource_t *io_space;
  pci_device_list_t devices;
} pci_bus_device_t;

static inline uint32_t pci_read_config(pci_device_t *device, unsigned reg,
                                       unsigned size) {
  return device->bus->read_config(device, reg, size);
}

static inline void pci_write_config(pci_device_t *device, unsigned reg,
                                    unsigned size, uint32_t value) {
  device->bus->write_config(device, reg, size, value);
}

static inline uint32_t pci_adjust_config(pci_device_t *device, unsigned reg,
                                         unsigned size, uint32_t value) {
  pci_write_config(device, reg, size, value);
  return pci_read_config(device, reg, size);
}

void pci_init();

#endif /* _SYS_PCI_H_ */
