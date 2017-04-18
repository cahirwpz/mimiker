#include <pci.h>
#include <stdc.h>
#include <malloc.h>
#include <pci_drivers.h>

static MALLOC_DEFINE(mp, "PCI bus discovery memory pool");

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

static int pci_bar_compare(const void *a, const void *b) {
  const pci_bar_t *bar0 = *(const pci_bar_t **)a;
  const pci_bar_t *bar1 = *(const pci_bar_t **)b;

  if (bar0->size < bar1->size)
    return 1;
  if (bar0->size > bar1->size)
    return -1;
  return 0;
}

void pci_bus_assign_space(pci_bus_t *pcibus, intptr_t mem_base,
                          intptr_t io_base) {
  /* Count PCI base address registers & allocate memory */
  unsigned nbars = 0;

  for (int j = 0; j < pcibus->ndevs; j++) {
    pci_device_t *pcidev = &pcibus->dev[j];
    nbars += pcidev->nbars;
  }

  pci_bar_t **bars = kmalloc(mp, sizeof(pci_bar_t *) * nbars, M_ZERO);

  for (int j = 0, n = 0; j < pcibus->ndevs; j++) {
    pci_device_t *pcidev = &pcibus->dev[j];
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
  }

  kfree(mp, bars);
}

int pci_bus_match(device_t *device, driver_t *driver) {
  pci_device_t *pcidev = device_to_pci_device(device);
  pci_driver_t *pcidrv = driver_to_pci_driver(driver);
  pci_devid_t **idptr = &pcidrv->devids;
  while (*idptr != NULL) {
    if (pcidev->vendor_id == (*idptr)->vendor &&
        pcidev->device_id == (*idptr)->device)
      return 1;
    idptr++;
  }
  return 0;
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

    if (pcidev->device.driver) {
      kprintf("   driver: %s\n", pcidev->device.driver->name);
    }
  }
}

bus_type_t pci_bus_type = {
  .name = "pci", .match = pci_bus_match};

static pci_bus_t pci_bus[1];

void pci_init() {
  vm_page_t *pg = pm_alloc(1);

  kmalloc_init(mp);
  kmalloc_add_arena(mp, pg->vaddr, PG_SIZE(pg));

  pci_bus->ndevs = platform_pci_bus_count();
  pci_bus->dev = kmalloc(mp, sizeof(pci_device_t) * pci_bus->ndevs, M_ZERO);
  platform_pci_bus_enumerate(pci_bus);

  for (int i = 0; i < pci_bus->ndevs; i++) {
    pci_device_t *pcidev = &pci_bus->dev[i];

    pcidev->device.bus = &pci_bus_type;
    snprintf(pcidev->device.bus_id, 100, "%02x:%02x.%02x", pcidev->addr.bus,
             pcidev->addr.device, pcidev->addr.function);
    device_register(&pcidev->device);
  }

  pci_bus_dump(pci_bus);
}
