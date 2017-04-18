#include <drivers.h>
#include <common.h>
#include <stdc.h>
#include <pci_drivers.h>

int bus_register(bus_type_t *bus) {
  log("Registering a bus");
  TAILQ_INIT(&bus->devices);
  TAILQ_INIT(&bus->drivers);
  return 0;
}
int device_register(device_t *device) {
  log("Registering a device");

  device->driver = NULL;
  device->driver_data = NULL;
  TAILQ_INIT(&device->children);

  bus_type_t *bus = device->bus;
  TAILQ_INSERT_TAIL(&bus->devices, device, bus_dev_entry);

  if (device->parent)
    TAILQ_INSERT_TAIL(&device->parent->children, device, children_entry);

  driver_t *driver;
  TAILQ_FOREACH (driver, &bus->drivers, bus_drv_entry) {
    int res = bus->match(device, driver);
    if (!res)
      continue;
    device->driver = driver;
    res = driver->probe(device);
    if (res)
      goto bound;
  }
  device->driver = NULL;
  return 1;

bound:

  TAILQ_INSERT_TAIL(&driver->devices, device, drv_dev_entry);

  return 0;
}

int driver_register(driver_t *driver) {
  log("Registering a driver");

  TAILQ_INIT(&driver->devices);
  TAILQ_INSERT_TAIL(&driver->bus->drivers, driver, bus_drv_entry);

  return 0;
}

extern pci_driver_t piix4_isa_driver;

void register_builtin_drivers() {

  /* TODO: Register built-in drivers, pci_drivers and buses using a linker set.
   */

  bus_register(&pci_bus_type);

  pci_driver_register(&piix4_isa_driver);
}
