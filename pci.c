#include <mips.h>
#include <malta.h>
#include <gt64120.h>
#include <libkern.h>
#include <pci.h>

#define PCI0_CFG_ADDR_R GT_R(GT_PCI0_CFG_ADDR)
#define PCI0_CFG_DATA_R GT_R(GT_PCI0_CFG_DATA)

#define PCI0_CFG_REG_SHIFT   2
#define PCI0_CFG_FUNCT_SHIFT 8
#define PCI0_CFG_DEV_SHIFT   11
#define PCI0_CFG_BUS_SHIFT   16
#define PCI0_CFG_ENABLE      0x80000000

#define PCI0_CFG_REG(dev, funct, reg)  \
  (((dev) << PCI0_CFG_DEV_SHIFT)     | \
   ((funct) << PCI0_CFG_FUNCT_SHIFT) | \
   ((reg) << PCI0_CFG_REG_SHIFT))

/* For reference look at: http://wiki.osdev.org/PCI */

static const pci_device_id *pci_find_device(
    const pci_vendor_id *vendor, uint16_t device_id)
{
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

void dump_pci0() {
  for (int j = 0; j < 32; j++) {
    for (int k = 0; k < 8; k++) {
      PCI0_CFG_ADDR_R = PCI0_CFG_ENABLE | PCI0_CFG_REG(j, k, 0);

      if (PCI0_CFG_DATA_R == -1)
        continue;

      uint16_t device_id = PCI0_CFG_DATA_R >> 16;
      uint16_t vendor_id = PCI0_CFG_DATA_R;

      PCI0_CFG_ADDR_R = PCI0_CFG_ENABLE | PCI0_CFG_REG(j, k, 2);

      uint8_t class = (PCI0_CFG_DATA_R & 0xff000000) >> 24;

      kprintf("[pci:0:%02x.%02x] %s:", j, k, pci_class_code[class]);

      const pci_vendor_id *vendor = pci_find_vendor(vendor_id);
      const pci_device_id *device = pci_find_device(vendor, device_id);

      if (vendor)
        kprintf(" %s", vendor->name);
      else
        kprintf(" vendor:$%04x", vendor_id);

      if (device)
        kprintf(" %s\n", device->name);
      else
        kprintf(" device:$%04x\n", device_id);

      PCI0_CFG_ADDR_R = PCI0_CFG_ENABLE | PCI0_CFG_REG(j, k, 15);

      uint8_t pin = PCI0_CFG_DATA_R >> 8;
      uint8_t irq = PCI0_CFG_DATA_R;

      if (pin)
        kprintf("[pci:0:%02x.%02x] Interrupt: pin %c routed to IRQ %d\n",
            j, k, 'A' + pin - 1, irq);

      for (int i = 0; i < 6; i++) {
        PCI0_CFG_ADDR_R = PCI0_CFG_ENABLE | PCI0_CFG_REG(j, k, 4 + i);
        uint32_t bar = PCI0_CFG_DATA_R;
        PCI0_CFG_DATA_R = -1;
        uint32_t size = PCI0_CFG_DATA_R;
        if (size == 0 || bar == size)
          continue;
        char *type;
        if (bar & 1) {
          bar &= ~3;
          size &= ~3;
          type = "I/O ports";
        } else {
          if (bar & 8)
            type = "Memory (prefetchable)";
          else
            type = "Memory (non-prefetchable)";
          bar &= ~15;
          size &= ~15;
        }
        kprintf("[pci:0:%02x.%02x] Region %d: %s at $%08x [size=$%x]\n",
            j, k, i, type, bar, -size);
      }
    }
  }
}
