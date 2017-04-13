#include <pci.h>
#include <pci_drivers.h>
#include <drivers.h>

#include <stdc.h>
#include <common.h>

int pci_driver_probe(device_t *device) {
  pci_device_t *pcidev = device_to_pci_device(device);
  pci_driver_t *pcidrv = driver_to_pci_driver(device->driver);
  pci_devid_t devid = {.vendor = pcidev->vendor_id,
                       .device = pcidev->device_id};
  return pcidrv->probe(pcidev, &devid);
}

int pci_driver_register(pci_driver_t *drv) {
  drv->driver.name = drv->name;
  drv->driver.bus = &pci_bus_type;
  drv->driver.probe = pci_driver_probe;

  return driver_register(&drv->driver);
}
