#define KL_LOG KL_DEV
#include <sys/klog.h>
#include <sys/mimiker.h>
#include <sys/device.h>
#include <sys/rman.h>

KMALLOC_DEFINE(M_DEV, "devices & drivers");

void device_init(device_t *dev, devclass_t *dc, int unit) {
  TAILQ_INIT(&dev->children);
  dev->parent = NULL;
  dev->unit = unit;
  dev->devclass = dc;
}

device_t *device_add_child(device_t *parent, devclass_t *dc, int unit) {
  device_t *child = kmalloc(M_DEV, sizeof(device_t), M_ZERO);
  device_init(child, dc, unit);
  child->parent = parent;
  TAILQ_INSERT_TAIL(&parent->children, child, link);
  return child;
}

device_t *device_identify(driver_t *driver, device_t *parent) {
  assert(driver != NULL);
  d_identify_t identify = driver->identify;
  return identify ? identify(driver, parent) : NULL;
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
