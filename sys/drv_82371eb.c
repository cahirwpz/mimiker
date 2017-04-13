#include <pci_drivers.h>

/* NOTE: This file implements a driver - we'll probably want to move it to a
   separate directory in the source tree .*/

#define PCI_VENDOR_INTEL 0x8086
#define PCI_DEVICE_82371EB_ISA 0x7110

static pci_devid_t piix4_isa_ids[] = {
  {PCI_VENDOR_INTEL, PCI_DEVICE_82371EB_ISA},
  {0, }
};

static int piix4_isa_probe(pci_device_t* dev, pci_devid_t* id);

pci_driver_t piix4_isa_driver = {
  .name = "piix4_isa",
  .devids = piix4_isa_ids,
  .probe = piix4_isa_probe
};

static int piix4_isa_probe(pci_device_t* dev, pci_devid_t* id){
  return 1;
}
