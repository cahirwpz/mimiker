#define KL_LOG KL_DEV
#include <stdc.h>
#include <klog.h>
#include <device.h>
#include <pci.h>
#include <sysinit.h>

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
  if (dev->driver->probe == NULL)
    return 0;
  return dev->driver->probe(dev);
}

int device_attach(device_t *dev) {
  dev->state = kmalloc(M_DEV, dev->driver->size, M_ZERO);
  return dev->driver->attach(dev);
}

int device_detach(device_t *dev) {
  if (dev->driver->detach == NULL)
    return 0;
  int res = dev->driver->detach(dev);
  if (res == 0)
    kfree(M_DEV, dev->state);
  return res;
}

extern pci_bus_driver_t gt_pci;

void driver_init() {
  /* TODO: a platform should expose root bus - probe & attach process should
   * start from it. */
  device_t *pcib = device_alloc();
  pcib->driver = &gt_pci.driver;
  device_attach(pcib);

  /* TODO: Should following code become a part of generic bus code? */
  device_t *dev;
  SET_DECLARE(driver_table, driver_t);
  klog("Scanning %s for known devices.", pcib->driver->desc);
  TAILQ_FOREACH (dev, &pcib->children, link) {
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
}
SYSINIT_ADD(driver, driver_init, DEPS("proc"));
