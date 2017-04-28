#include <stdc.h>
#include <malloc.h>
#include <pci.h>

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

static void pci_bus_enumerate(pci_bus_device_t *pcib) {
  for (int j = 0; j < 32; j++) {
    for (int k = 0; k < 8; k++) {
      if (!pci_device_present(pcib->bus, 0, j, k))
        continue;

      pci_device_t *pcidev = kmalloc(mp, sizeof(pci_device_t), M_ZERO);
      pcidev->bus = pcib->bus;
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

        unsigned type, flags;

        if (addr & PCI_BAR_IO) {
          type = RT_IOPORTS;
          flags = RF_NONE;
          size &= ~PCI_BAR_IO_MASK;
        } else {
          type = RT_MEMORY;
          flags = (addr & PCI_BAR_PREFETCHABLE) ? RF_PREFETCHABLE : RF_NONE;
          size &= ~PCI_BAR_MEMORY_MASK;
        }

        size = -size;
        resource_t *bar = &pcidev->bar[pcidev->nbars++];
        *bar = (resource_t){.r_owner = pcidev,
                            .r_type = type,
                            .r_flags = flags,
                            .r_start = 0,
                            .r_end = size - 1,
                            .r_id = i};
      }

      TAILQ_INSERT_TAIL(&pcib->devices, pcidev, link);
    }
  }
}

static int pci_bar_compare(const void *a, const void *b) {
  const resource_t *bar0 = *(const resource_t **)a;
  const resource_t *bar1 = *(const resource_t **)b;

  if (bar0->r_end < bar1->r_end)
    return 1;
  if (bar0->r_end > bar1->r_end)
    return -1;
  return 0;
}

static void pci_bus_assign_space(pci_bus_device_t *pcib) {
  /* Count PCI base address registers & allocate memory */
  unsigned nbars = 0, ndevs = 0;
  pci_device_t *pcidev;

  TAILQ_FOREACH (pcidev, &pcib->devices, link) {
    nbars += pcidev->nbars;
    ndevs++;
  }

  log("devs = %d, nbars = %d", ndevs, nbars);

  resource_t **bars = kmalloc(mp, sizeof(resource_t *) * nbars, M_ZERO);
  unsigned n = 0;

  TAILQ_FOREACH (pcidev, &pcib->devices, link) {
    for (int i = 0; i < pcidev->nbars; i++)
      bars[n++] = &pcidev->bar[i];
  }

  qsort(bars, nbars, sizeof(resource_t *), pci_bar_compare);

  intptr_t io_base = pcib->io_space->r_start;
  intptr_t mem_base = pcib->mem_space->r_start;

  for (int j = 0; j < nbars; j++) {
    resource_t *bar = bars[j];
    if (bar->r_type == RT_IOPORTS) {
      bar->r_bus_space = pcib->io_space->r_bus_space;
      bar->r_start += io_base;
      bar->r_end += io_base;
      io_base = bar->r_end + 1;
    } else if (bar->r_type == RT_MEMORY) {
      bar->r_bus_space = pcib->mem_space->r_bus_space;
      bar->r_start += mem_base;
      bar->r_end += mem_base;
      mem_base = bar->r_end + 1;
    }

    /* Write the BAR address back to PCI bus config. It's safe to write the
     * entire address without masking bits - only base address bits are
     * writable. */
    pci_write_config(bar->r_owner, PCIR_BAR(bar->r_id), 4, bar->r_start);
  }

  kfree(mp, bars);
}

static void pci_bus_dump(pci_bus_device_t *pcib) {
  pci_device_t *pcidev;

  TAILQ_FOREACH (pcidev, &pcib->devices, link) {
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
      resource_t *bar = &pcidev->bar[i];
      char *type;

      if (bar->r_type == RT_IOPORTS) {
        type = "I/O ports";
      } else {
        type = (bar->r_flags & RF_PREFETCHABLE) ? "Memory (prefetchable)"
                                                : "Memory (non-prefetchable)";
      }
      kprintf("%s Region %d: %s at %p [size=$%x]\n", devstr, i, type,
              (void *)bar->r_start, (unsigned)(bar->r_end - bar->r_start + 1));
    }
  }
}

extern pci_bus_device_t gt_pci;

void pci_init() {
  kmalloc_init(mp, 1, 1);

  /* TODO: actions below will become a part of `device_attach` function of
   * generic driver for PCI bus. */
  pci_bus_enumerate(&gt_pci);
  pci_bus_assign_space(&gt_pci);
  pci_bus_dump(&gt_pci);
}
