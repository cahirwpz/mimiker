#define KL_LOG KL_DEV
#include <klog.h>
#include <device.h>
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

int __attribute__((unused)) device_probe2(device_t *dev) {
  device_t *parent = dev->parent;
  devclass_t *dc = devclass_find(parent->driver->desc);
  klog("devclass found %s\n", dc->name);
  driver_t **drv;
  // bledy > 0
  // matching <=0. 0 = best driver
  driver_t *best_drv = NULL;
  int best = INT_MIN;

  DEVCLASS_FOREACH(drv, dc) {
    dev->driver = *drv;
    d_probe_t probe = dev->driver->probe;
    int found = probe ? probe(dev) : 1; // 1 means no probe method
    if (found > 0) {
      klog("no probe method!");
    } else {
      if (found > best) {
        best = found;
        best_drv = *drv;
      }
    }
  }

  if (best_drv) {
    dev->driver = best_drv;
    return 0;
  }

  return -1; // probe failed
}

int device_probe_and_attach(device_t *dev) {
  int error = device_probe2(dev);
  if (error == -1) {
    klog("error");
    return error;
  }
  // best driver przypisany
  error = device_attach(dev);
  return error;
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

// this function serves as an ugly hack.
device_t *make_device(device_t *parent, driver_t *driver) {
  device_t *dev = device_add_child(parent);
  dev->driver = driver;
  if (device_probe(dev))
    device_attach(dev);
  return dev;
}

void device_add_resource(device_t *dev, resource_t *r, int rid) {
  r->r_owner = dev;
  r->r_id = rid;
  TAILQ_INSERT_HEAD(&dev->resources, r, r_device);
}
