#include <sys/libkern.h>
#include <sys/malloc.h>
#include <sys/device.h>
#include <sys/devclass.h>
#include <sys/pci.h>

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

static bool pci_device_present(device_t *pcid) {
  return pci_read_config(pcid, PCIR_DEVICEID, 4) != -1U;
}

static int pci_device_nfunctions(device_t *pcid) {
  if (!pci_device_present(pcid))
    return 0;
  uint32_t hdrtype = pci_read_config(pcid, PCIR_HEADERTYPE, 1);
  return (hdrtype & PCIH_HDR_MF) ? PCI_FUN_MAX_NUM : 1;
}

static uint32_t pci_bar_size(device_t *pcid, int bar, uint32_t addr) {
  pci_write_config(pcid, PCIR_BAR(bar), 4, -1U);
  uint32_t res = pci_read_config(pcid, PCIR_BAR(bar), 4);
  /* The original value of the BAR should be restored. */
  pci_write_config(pcid, PCIR_BAR(bar), 4, addr);
  return res;
}

DEVCLASS_CREATE(pci);

#define PCIA(b, d, f)                                                          \
  (pci_addr_t) {                                                               \
    .bus = (b), .device = (d), .function = (f)                                 \
  }
#define SET_PCIA(pcid, b, d, f)                                                \
  (((pci_device_t *)(pcid)->instance)->addr = PCIA((b), (d), (f)))

void pci_bus_enumerate(device_t *pcib) {
  device_t pcid = {.parent = pcib,
                   .bus = DEV_BUS_PCI,
                   .instance = (pci_device_t[1]){},
                   .state = NULL};

  for (int d = 0; d < PCI_DEV_MAX_NUM; d++) {
    SET_PCIA(&pcid, 0, d, 0);
    /* Note that if we don't check the MF bit of the device
     * and scan all functions, then some single-function devices
     * will report details for "fucntion 0" for every function. */
    int max_fun = pci_device_nfunctions(&pcid);

    for (int f = 0; f < max_fun; f++) {
      SET_PCIA(&pcid, 0, d, f);
      if (!pci_device_present(&pcid))
        continue;

      /* It looks like dev is a leaf in device tree, but it can also be an inner
       * node. */
      device_t *dev = device_add_child(pcib, -1);
      pci_device_t *pcid = kmalloc(M_DEV, sizeof(pci_device_t), M_ZERO);

      dev->bus = DEV_BUS_PCI;
      dev->instance = pcid;

      pcid->addr = PCIA(0, d, f);
      pcid->device_id = pci_read_config(dev, PCIR_DEVICEID, 2);
      pcid->vendor_id = pci_read_config(dev, PCIR_VENDORID, 2);
      pcid->class_code = pci_read_config(dev, PCIR_CLASSCODE, 1);
      pcid->pin = pci_read_config(dev, PCIR_IRQPIN, 1);
      pcid->irq = pci_read_config(dev, PCIR_IRQLINE, 1);

      /* XXX: we assume here that `dev` is a general PCI device
       * (i.e. header type = 0x00) and therefore has six bars. */
      for (int i = 0; i < PCI_BAR_MAX; i++) {
        uint32_t addr = pci_read_config(dev, PCIR_BAR(i), 4);
        uint32_t size = pci_bar_size(dev, i, addr);

        if (size == 0 || addr == size)
          continue;

        unsigned type, flags = 0;

        if (addr & PCI_BAR_IO) {
          type = RT_IOPORTS;
          size &= ~PCI_BAR_IO_MASK;
        } else {
          type = RT_MEMORY;
          if (addr & PCI_BAR_PREFETCHABLE)
            flags |= RF_PREFETCHABLE;
          size &= ~PCI_BAR_MEMORY_MASK;
        }

        size = -size;
        uint8_t id = pcid->nbars;
        pcid->bar[id] = (pci_bar_t){
          .owner = dev, .type = type, .flags = flags, .size = size, .rid = id};

        /* skip ISA I/O ports range */
        rman_addr_t start = (type == RT_IOPORTS) ? IO_ISASIZE : 0;

        device_add_resource(dev, type, id, start, RMAN_ADDR_MAX, size, flags);

        pcid->nbars++;
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

    for (int i = 0; i < PCI_BAR_MAX; i++) {
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
