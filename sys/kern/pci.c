#include <sys/libkern.h>
#include <sys/malloc.h>
#include <sys/device.h>
#include <sys/pci.h>

/* For reference look at: http://wiki.osdev.org/PCI */

const pci_device_id *pci_find_device(const pci_vendor_id *vendor,
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

const pci_vendor_id *pci_find_vendor(uint16_t vendor_id) {
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
                   .bus = DEV_BUS_PCI,
                   .instance = (pci_device_t[1]){{.addr = {bus, dev, func}}},
                   .state = NULL};
  return (pci_read_config(&pcid, PCIR_DEVICEID, 4) != 0xffffffff);
}

void pci_bus_enumerate(device_t *pcib) {
  for (int j = 0; j < 32; j++) {
    for (int k = 0; k < 8; k++) {
      if (!pci_device_present(pcib, 0, j, k))
        continue;

      device_t *dev = device_add_child(pcib, NULL, -1);
      pci_device_t *pcid = kmalloc(M_DEV, sizeof(pci_device_t), M_ZERO);

      dev->bus = DEV_BUS_PCI;
      dev->instance = pcid;

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
        pcid->bar[i] = (pci_bar_t){
          .owner = dev, .type = type, .flags = flags, .size = size, .rid = i};
      }
    }
  }

  pci_bus_dump(pcib);
}

/* TODO: to be replaced with GDB python script */
void pci_bus_dump(device_t *pcib) {
  device_t *dev;

  TAILQ_FOREACH (dev, &pcib->children, link) {
    pci_device_t *pcid = pci_device_of(dev);

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

    for (int i = 0; i < 6; i++) {
      pci_bar_t *bar = &pcid->bar[i];
      char *type;

      if (bar->size == 0)
        continue;

      if (bar->type == RT_IOPORTS) {
        type = "I/O ports";
      } else {
        type = (bar->flags & RF_PREFETCHABLE) ? "Memory (prefetchable)"
                                              : "Memory (non-prefetchable)";
      }
      kprintf("%s Region %x: %s [size=$%lx]\n", devstr, i, type, bar->size);
    }
  }
}
