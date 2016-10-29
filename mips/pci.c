#include <stdc.h>
#include <mips/mips.h>
#include <mips/malta.h>
#include <mips/gt64120.h>
#include <malloc.h>
#include <pci.h>

#define PCI0_CFG_ADDR_R GT_R(GT_PCI0_CFG_ADDR)
#define PCI0_CFG_DATA_R GT_R(GT_PCI0_CFG_DATA)

#define PCI0_CFG_REG_SHIFT 2
#define PCI0_CFG_FUNCT_SHIFT 8
#define PCI0_CFG_DEV_SHIFT 11
#define PCI0_CFG_BUS_SHIFT 16
#define PCI0_CFG_ENABLE 0x80000000

#define PCI0_CFG_REG(dev, funct, reg)                                          \
  (((dev) << PCI0_CFG_DEV_SHIFT) | ((funct) << PCI0_CFG_FUNCT_SHIFT) |         \
   ((reg) << PCI0_CFG_REG_SHIFT))

/* For reference look at: http://wiki.osdev.org/PCI */

static const pci_device_id *pci_find_device(const pci_vendor_id *vendor,
                                            uint16_t device_id) {
  if (vendor) {
    const pci_device_id *device = vendor->devices;
    while (device->name) {
      if (device->id == device_id)
        return device;
      device++;
    }
  }
  return NULL;
}

static const pci_vendor_id *pci_find_vendor(uint16_t vendor_id) {
  const pci_vendor_id *vendor = pci_vendor_list;
  while (vendor->name) {
    if (vendor->id == vendor_id)
      return vendor;
    vendor++;
  }
  return NULL;
}

static void pci_bus_enumerate(pci_bus_t *pcibus) {
  pcibus->dev = kernel_sbrk(0);
  pcibus->ndevs = 0;

  for (int j = 0; j < 32; j++) {
    for (int k = 0; k < 8; k++) {
      PCI0_CFG_ADDR_R = PCI0_CFG_ENABLE | PCI0_CFG_REG(j, k, 0);

      if (PCI0_CFG_DATA_R == -1)
        continue;

      pci_device_t *pcidev = kernel_sbrk(sizeof(pci_device_t));

      pcidev->addr.bus = 0;
      pcidev->addr.device = j;
      pcidev->addr.function = k;
      pcidev->device_id = PCI0_CFG_DATA_R >> 16;
      pcidev->vendor_id = PCI0_CFG_DATA_R;

      PCI0_CFG_ADDR_R = PCI0_CFG_ENABLE | PCI0_CFG_REG(j, k, 2);
      pcidev->class_code = (PCI0_CFG_DATA_R & 0xff000000) >> 24;

      PCI0_CFG_ADDR_R = PCI0_CFG_ENABLE | PCI0_CFG_REG(j, k, 15);
      pcidev->pin = PCI0_CFG_DATA_R >> 8;
      pcidev->irq = PCI0_CFG_DATA_R;

      for (int i = 0; i < 6; i++) {
        PCI0_CFG_ADDR_R =
          PCI0_CFG_ENABLE |
          PCI0_CFG_REG(pcidev->addr.device, pcidev->addr.function, 4 + i);
        unsigned addr = PCI0_CFG_DATA_R;
        PCI0_CFG_DATA_R = -1;
        unsigned size = PCI0_CFG_DATA_R;
        if (size == 0 || addr == size)
          continue;

        size &= (addr & PCI_BAR_IO) ? ~PCI_BAR_IO_MASK : ~PCI_BAR_MEMORY_MASK;
        size = -size;

        pci_bar_t *bar = &pcidev->bar[pcidev->nbars++];
        bar->addr = addr;
        bar->size = size;
      }

      pcibus->ndevs++;
    }
  }
}

static int pci_bar_compare(const void *a, const void *b) {
  const pci_bar_t *bar0 = *(const pci_bar_t **)a;
  const pci_bar_t *bar1 = *(const pci_bar_t **)b;

  if (bar0->size < bar1->size)
    return 1;
  if (bar0->size > bar1->size)
    return -1;
  return 0;
}

static void pci_bus_assign_space(pci_bus_t *pcibus, intptr_t mem_base,
                                 intptr_t io_base) {
  pci_bar_t **bars = kernel_sbrk(0);
  unsigned nbars = 0;

  for (int j = 0; j < pcibus->ndevs; j++) {
    pci_device_t *pcidev = &pcibus->dev[j];

    for (int i = 0; i < pcidev->nbars; i++) {
      void *ptr __attribute__((unused));
      ptr = kernel_sbrk(sizeof(pci_bar_t *));
      bars[nbars++] = &pcidev->bar[i];
    }
  }

  qsort(bars, nbars, sizeof(pci_bar_t *), pci_bar_compare);

  for (int j = 0; j < nbars; j++) {
    pci_bar_t *bar = bars[j];
    if (bar->addr & PCI_BAR_IO) {
      bar->addr |= io_base;
      io_base += bar->size;
    } else {
      bar->addr |= mem_base;
      mem_base += bar->size;
    }
  }

  kernel_brk(bars);
}

static void pci_bus_dump(pci_bus_t *pcibus) {
  for (int j = 0; j < pcibus->ndevs; j++) {
    pci_device_t *pcidev = &pcibus->dev[j];
    char devstr[16];

    snprintf(devstr, sizeof(devstr), "[pci:%02x:%02x.%02x]", pcidev->addr.bus,
             pcidev->addr.device, pcidev->addr.function);

    const pci_vendor_id *vendor = pci_find_vendor(pcidev->vendor_id);
    const pci_device_id *device = pci_find_device(vendor, pcidev->device_id);

    kprintf("%s %s", devstr, pci_class_code[pcidev->class_code]);

    if (vendor)
      kprintf(" %s", vendor->name);
    else
      kprintf(" vendor:$%04x", pcidev->vendor_id);

    if (device)
      kprintf(" %s\n", device->name);
    else
      kprintf(" device:$%04x\n", pcidev->device_id);

    if (pcidev->pin)
      kprintf("%s Interrupt: pin %c routed to IRQ %d\n", devstr,
              'A' + pcidev->pin - 1, pcidev->irq);

    for (int i = 0; i < pcidev->nbars; i++) {
      pci_bar_t *bar = &pcidev->bar[i];
      pm_addr_t addr = bar->addr;
      size_t size = bar->size;
      char *type;

      if (addr & PCI_BAR_IO) {
        addr &= ~PCI_BAR_IO_MASK;
        type = "I/O ports";
      } else {
        if (addr & PCI_BAR_PREFETCHABLE)
          type = "Memory (prefetchable)";
        else
          type = "Memory (non-prefetchable)";
        addr &= ~PCI_BAR_MEMORY_MASK;
      }
      kprintf("%s Region %d: %s at %p [size=$%x]\n", devstr, i, type,
              (void *)addr, (unsigned)size);
    }
  }
}

static pci_bus_t pci_bus[1];

void pci_init() {
  pci_bus_enumerate(pci_bus);
  pci_bus_assign_space(pci_bus, MALTA_PCI0_MEMORY_BASE, PCI_IO_SPACE_BASE);
  pci_bus_dump(pci_bus);
}
