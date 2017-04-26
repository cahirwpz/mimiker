#include <stdc.h>
#include <mips/mips.h>
#include <mips/malta.h>
#include <malloc.h>
#include <pci.h>

static pci_device_list_t pci_devices;

static MALLOC_DEFINE(mp, "PCI bus discovery memory pool");

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

static bool pci_device_present(pci_bus_t *pcib, unsigned bus, unsigned dev,
                               unsigned func) {
  pci_device_t pcidev = {.bus = pcib, .addr = {bus, dev, func}};
  return (pci_read_config(&pcidev, PCIR_DEVICEID, 4) != 0xffffffff);
}

static void pci_bus_enumerate(pci_device_list_t *devlst, pci_bus_t *pcib) {
  for (int j = 0; j < 32; j++) {
    for (int k = 0; k < 8; k++) {
      if (!pci_device_present(pcib, 0, j, k))
        continue;

      pci_device_t *pcidev = kmalloc(mp, sizeof(pci_device_t), M_ZERO);
      pcidev->bus = pcib;
      pcidev->addr = (pci_addr_t){0, j, k};
      pcidev->device_id = pci_read_config(pcidev, PCIR_DEVICEID, 2);
      pcidev->vendor_id = pci_read_config(pcidev, PCIR_VENDORID, 2);
      pcidev->class_code = pci_read_config(pcidev, PCIR_CLASSCODE, 1);
      pcidev->pin = pci_read_config(pcidev, PCIR_IRQPIN, 1);
      pcidev->irq = pci_read_config(pcidev, PCIR_IRQLINE, 1);

      for (int i = 0; i < 6; i++) {
        uint32_t addr = pci_read_config(pcidev, PCIR_BAR(i), 4);
        uint32_t size = pci_adjust_config(pcidev, PCIR_BAR(i), 4, 0xffffffff);

        if (size == 0 || addr == size)
          continue;

        size &= (addr & PCI_BAR_IO) ? ~PCI_BAR_IO_MASK : ~PCI_BAR_MEMORY_MASK;
        size = -size;
        pcidev->bar[pcidev->nbars++] =
          (pci_bar_t){.addr = addr, .size = size, .dev = pcidev, .i = i};
      }

      TAILQ_INSERT_TAIL(devlst, pcidev, link);
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

static void pci_bus_assign_space(pci_device_list_t *devlst, intptr_t mem_base,
                                 intptr_t io_base) {
  /* Count PCI base address registers & allocate memory */
  unsigned nbars = 0, ndevs = 0;
  pci_device_t *pcidev;

  TAILQ_FOREACH(pcidev, devlst, link) {
    nbars += pcidev->nbars;
    ndevs++;
  }

  log("devs = %d, nbars = %d", ndevs, nbars);

  pci_bar_t **bars = kmalloc(mp, sizeof(pci_bar_t *) * nbars, M_ZERO);
  unsigned n = 0;

  TAILQ_FOREACH(pcidev, devlst, link) {
    for (int i = 0; i < pcidev->nbars; i++)
      bars[n++] = &pcidev->bar[i];
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

    /* Write the BAR address back to PCI bus config. It's safe to write the
     * entire address without masking bits - only base address bits are
     * writable. */
    pci_write_config(bar->dev, PCIR_BAR(bar->i), 4, bar->addr);
  }

  kfree(mp, bars);
}

static void pci_bus_dump(pci_device_list_t *devlst) {
  pci_device_t *pcidev;

  TAILQ_FOREACH(pcidev, devlst, link) {
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

PCI_BUS_DECLARE(gt_pci_bus);

void pci_init() {
  TAILQ_INIT(&pci_devices);
  kmalloc_init(mp, 1, 1);

  pci_bus_enumerate(&pci_devices, gt_pci_bus);
  pci_bus_assign_space(&pci_devices, MALTA_PCI0_MEMORY_BASE, PCI_IO_SPACE_BASE);
  pci_bus_dump(&pci_devices);
}
