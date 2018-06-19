#define KL_LOG KL_DEV
#include <klog.h>
#include <device.h>
#include <pci.h>
#include <rman.h>

int bus_generic_probe(device_t *bus) {
  device_t *dev;
  SET_DECLARE(driver_table, driver_t);
  klog("Scanning %s for known devices.", bus->driver->desc);
  TAILQ_FOREACH (dev, &bus->children, link) {
    driver_t **drv_p;
    SET_FOREACH(drv_p, driver_table) {
      driver_t *drv = *drv_p;
      dev->driver = drv;
      if (device_probe(dev)) {
        klog("%s detected!", drv->desc);
        device_attach(dev);
        break;
      }
      dev->driver = NULL;
    }
  }
  return 0;
}
