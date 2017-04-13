#include <mips/mips.h>
#include <mips/malta.h>
#include <mips/gt64120.h>
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

unsigned platform_pci_bus_count() {
  unsigned ndevs = 0;

  for (int j = 0; j < 32; j++) {
    for (int k = 0; k < 8; k++) {
      PCI0_CFG_ADDR_R = PCI0_CFG_ENABLE | PCI0_CFG_REG(j, k, 0);

      if (PCI0_CFG_DATA_R == -1)
        continue;

      ndevs++;
    }
  }
  return ndevs;
}

void platform_pci_bus_enumerate(pci_bus_t *pcibus) {

  /* read device descriptions into main memory */
  for (int j = 0, n = 0; j < 32; j++) {
    for (int k = 0; k < 8; k++) {
      PCI0_CFG_ADDR_R = PCI0_CFG_ENABLE | PCI0_CFG_REG(j, k, 0);

      if (PCI0_CFG_DATA_R == -1)
        continue;

      pci_device_t *pcidev = &pcibus->dev[n++];

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
    }
  }

  pci_bus_assign_space(pcibus, MALTA_PCI0_MEMORY_BASE, PCI_IO_SPACE_BASE);
}
