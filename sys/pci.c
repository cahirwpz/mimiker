#include <stdc.h>
#include <malloc.h>
#include <device.h>
#include <pci.h>

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

static bool pci_device_present(device_t *pcib, unsigned bus, unsigned dev,
                               unsigned func) {
  device_t pcid = {.parent = pcib,
                   .instance = (pci_dev_data_t[1]){{.addr = {bus, dev, func}}}};
  return (pci_read_config(&pcid, PCIR_DEVICEID, 4) != 0xffffffff);
}

void pci_bus_enumerate(device_t *pcib) {
  for (int j = 0; j < 32; j++) {
    for (int k = 0; k < 8; k++) {
      if (!pci_device_present(pcib, 0, j, k))
        continue;

      device_t *dev = device_add_child(pcib);
      dev->instance = kmalloc(M_DEV, sizeof(pci_dev_data_t), M_ZERO);

      pci_dev_data_t *pcid = dev->instance;
      pcid->addr = (pci_addr_t){0, j, k};
      pcid->device_id = pci_read_config(dev, PCIR_DEVICEID, 2);
      pcid->vendor_id = pci_read_config(dev, PCIR_VENDORID, 2);
      pcid->class_code = pci_read_config(dev, PCIR_CLASSCODE, 1);
      pcid->pin = pci_read_config(dev, PCIR_IRQPIN, 1);
      pcid->irq = pci_read_config(dev, PCIR_IRQLINE, 1);

      for (int i = 0; i < 6; i++) {
        uint32_t addr = pci_read_config(dev, PCIR_BAR(i), 4);
        uint32_t size = pci_adjust_config(dev, PCIR_BAR(i), 4, 0xffffffff);

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
        resource_t *bar = &pcid->bar[pcid->nbars++];
        *bar = (resource_t){.r_owner = dev,
                            .r_type = type,
                            .r_flags = flags,
                            .r_start = 0,
                            .r_end = size - 1,
                            .r_id = i};
      }
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

void pci_bus_assign_space(device_t *pcib) {
  /* Count PCI base address registers & allocate memory */
  unsigned nbars = 0, ndevs = 0;
  device_t *dev;

  TAILQ_FOREACH (dev, &pcib->children, link) {
    pci_dev_data_t *pcid = dev->instance;
    nbars += pcid->nbars;
    ndevs++;
  }

  resource_t **bars = kmalloc(M_DEV, sizeof(resource_t *) * nbars, M_ZERO);
  unsigned n = 0;

  TAILQ_FOREACH (dev, &pcib->children, link) {
    pci_dev_data_t *pcid = dev->instance;
    for (int i = 0; i < pcid->nbars; i++)
      bars[n++] = &pcid->bar[i];
  }

  qsort(bars, nbars, sizeof(resource_t *), pci_bar_compare);

  pci_bus_state_t *data = pcib->state;
  intptr_t io_base = data->io_space->r_start;
  intptr_t mem_base = data->mem_space->r_start;

  for (int j = 0; j < nbars; j++) {
    resource_t *bar = bars[j];
    if (bar->r_type == RT_IOPORTS) {
      bar->r_bus_space = data->io_space->r_bus_space;
      bar->r_start += io_base;
      bar->r_end += io_base;
      io_base = bar->r_end + 1;
    } else if (bar->r_type == RT_MEMORY) {
      bar->r_bus_space = data->mem_space->r_bus_space;
      bar->r_start += mem_base;
      bar->r_end += mem_base;
      mem_base = bar->r_end + 1;
    }

    /* Write the BAR address back to PCI bus config. It's safe to write the
     * entire address without masking bits - only base address bits are
     * writable. */
    pci_write_config(bar->r_owner, PCIR_BAR(bar->r_id), 4, bar->r_start);
  }

  kfree(M_DEV, bars);
}

void pci_bus_dump(device_t *pcib) {
  device_t *dev;

  TAILQ_FOREACH (dev, &pcib->children, link) {
    pci_dev_data_t *pcid = dev->instance;

    char devstr[16];

    snprintf(devstr, sizeof(devstr), "[pci:%02x:%02x.%02x]", pcid->addr.bus,
             pcid->addr.device, pcid->addr.function);

    const pci_vendor_id *vendor = pci_find_vendor(pcid->vendor_id);
    const pci_device_id *device = pci_find_device(vendor, pcid->device_id);

    kprintf("%s %s", devstr, pci_class_code[pcid->class_code]);

    if (vendor)
      kprintf(" %s", vendor->name);
    else
      kprintf(" vendor:$%04x", pcid->vendor_id);

    if (device)
      kprintf(" %s\n", device->name);
    else
      kprintf(" device:$%04x\n", pcid->device_id);

    if (pcid->pin)
      kprintf("%s Interrupt: pin %c routed to IRQ %d\n", devstr,
              'A' + pcid->pin - 1, pcid->irq);

    for (int i = 0; i < pcid->nbars; i++) {
      resource_t *bar = &pcid->bar[i];
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
