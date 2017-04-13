#include <pci.h>
#include <pci_drivers.h>

#include <stdc.h>
#include <common.h>

int pci_driver_probe(device_t *device) {
  // Perform pci-specific probing
  return 0;
}

int pci_driver_register(pci_driver_t *drv) {
  drv->driver.name = drv->name;
  drv->driver.bus = &pci_bus_type;
  drv->driver.probe = pci_driver_probe;

  driver_register(&drv->driver);
  return 0;
}

// Example pci driver
pci_driver_t vga_driver = {.name = "vga"};
