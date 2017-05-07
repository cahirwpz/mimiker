#define KL_LOG KL_DEV
#include <stdc.h>
#include <klog.h>
#include <device.h>
#include <pci.h>

MALLOC_DEFINE(M_DEV, "device/driver pool", 128, 1024);

static device_t *device_alloc() {
  device_t *dev = kmalloc(M_DEV, sizeof(device_t), M_ZERO);
  TAILQ_INIT(&dev->children);
  return dev;
}

device_t *device_add_child(device_t *dev) {
  device_t *child = device_alloc();
  child->parent = dev;
  TAILQ_INSERT_TAIL(&dev->children, child, link);
  return child;
}

int device_probe(device_t *dev) {
  return dev->driver->probe ? dev->driver->probe(dev) : 0;
}

int device_attach(device_t *dev) {
  dev->state = kmalloc(M_DEV, dev->driver->size, M_ZERO);
  return dev->driver->attach(dev);
}

int device_detach(device_t *dev) {
  int res = dev->driver->detach(dev);
  if (res == 0)
    kfree(M_DEV, dev->state);
  return res;
}

/* Normally this file an intialization procedure would not exits! This is barely
   a substitute for currently unimplemented device-driver matching mechanism. */
extern pci_bus_driver_t gt_pci;

void driver_init() {
  device_t *pcib = device_alloc();
  pcib->driver = &gt_pci.driver;
  device_attach(pcib);

  device_t *dev;
  SET_DECLARE(driver_table, driver_t);
  log("Scanning %s for known devices.", pcib->driver->desc);
  TAILQ_FOREACH (dev, &pcib->children, link) {
    driver_t **drv_p;
    SET_FOREACH(drv_p, driver_table) {
      driver_t *drv = *drv_p;
      dev->driver = drv;
      if (device_probe(dev)) {
        log("%s detected!", drv->desc);
        device_attach(dev);
        break;
      }
      dev->driver = NULL;
    }
  }
}
