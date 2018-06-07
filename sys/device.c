#define KL_LOG KL_DEV
#include <stdc.h>
#include <klog.h>
#include <device.h>
#include <pci.h>
#include <rman.h>

MALLOC_DEFINE(M_DEV, "devices & drivers", 128, 1024);

static device_t *device_alloc(void) {
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

/* TODO: this routine should go over all drivers within a suitable class and
 * choose the best driver. For now the user is responsible for setting the
 * driver before calling `device_probe`. */
int device_probe(device_t *dev) {
  assert(dev->driver != NULL);
  d_probe_t probe = dev->driver->probe;
  int found = probe ? probe(dev) : 1;
  if (found)
    dev->state = kmalloc(M_DEV, dev->driver->size, M_ZERO);
  return found;
}

int device_attach(device_t *dev) {
  assert(dev->driver != NULL);
  d_attach_t attach = dev->driver->attach;
  return attach ? attach(dev) : 0;
}

int device_detach(device_t *dev) {
  assert(dev->driver != NULL);
  d_detach_t detach = dev->driver->detach;
  int res = detach ? detach(dev) : 0;
  if (res == 0)
    kfree(M_DEV, dev->state);
  return res;
}

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

device_t *make_device(device_t *parent, driver_t *driver) {
  device_t *dev = device_add_child(parent);
  dev->driver = driver;
  if (device_probe(dev))
    device_attach(dev);
  return dev;
}

/* what about rtc devices? */
void dump_device_tree_resources(device_t *dev, int nest) {
  if (!dev->driver)
    return;
  device_t *child;
  resource_t *r;
  kprintf("%s resources:\n", dev->driver->desc);
  LIST_FOREACH(r, &dev->resources, r_device) {
    kprintf("start: %x, end: %x\n", r->r_start, r->r_end);
  }
  TAILQ_FOREACH (child, &dev->children, link) {
    dump_device_tree_resources(child, nest + 1);
  }
}
