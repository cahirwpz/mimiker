#define KL_LOG KL_DEV
#include <klog.h>
#include <device.h>
#include <rman.h>
#include <devclass.h>

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

device_t *device_add_nameunit_child(device_t *parentbus, char *name, int unit){
  device_t *child;
  child = device_add_child(parentbus);
  child->name = name;
  child->unit = unit;
  return child;
}

// similar to device_probe and device_probe_child from freebsd
int device_probe(device_t *dev) {
  device_t *parent = dev->parent;
  // search devclass by driver or by device name?
  devclass_t *dc = devclass_find(parent->name);
  klog("devclass found %s\n", dc->name);
  driver_t **drv;
  // bledy > 0
  // matching <=0. 0 = best driver
  driver_t *best_drv = NULL;
  int best = BUS_PROBE_NOMATCH;

  DEVCLASS_FOREACH(drv, dc) {
    dev->driver = *drv;
    d_probe_t probe = dev->driver->probe;
    int found = probe ? probe(dev) : BUS_NO_METHOD;

    if (found > 0)
      continue;
    if (found == 0)
      return BUS_PROBE_SPECIFIC;
    if (found > best) {
      best = found;
      best_drv = *drv;
    }
  }

  if (best_drv > BUS_PROBE_NOMATCH && best_drv <=0) {
    dev->driver = best_drv;
    return best;
  }

  return 1; // probe failed
}

int device_probe_and_attach(device_t *dev) {
  int error;
  error = device_probe2(dev);

  if (error > 0)
    return error;

  // best driver przypisany
  error = device_attach(dev);
  return error;
}

int device_attach(device_t *dev) {
  assert(dev->driver != NULL);
  d_attach_t attach = dev->driver->attach;
  return attach ? attach(dev) : BUS_NO_METHOD;
}


int device_detach(device_t *dev) {
  assert(dev->driver != NULL);
  d_detach_t detach = dev->driver->detach;
  int res = detach ? detach(dev) : BUS_NO_METHOD;
  // czy tylko wtedy chcemy wolac kfree?
  // nie zawsze?
  if (res == BUS_NO_METHOD)
    kfree(M_DEV, dev->state);
  return res;
}

void device_add_resource(device_t *dev, resource_t *r, int rid) {
  r->r_owner = dev;
  r->r_id = rid;
  TAILQ_INSERT_HEAD(&dev->resources, r, r_device);
}
