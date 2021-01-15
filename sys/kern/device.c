#define KL_LOG KL_DEV
#include <sys/klog.h>
#include <sys/errno.h>
#include <sys/mimiker.h>
#include <sys/device.h>
#include <sys/rman.h>
#include <sys/bus.h>

KMALLOC_DEFINE(M_DEV, "devices & drivers");

device_t *device_alloc(int unit) {
  device_t *dev = kmalloc(M_DEV, sizeof(device_t), M_ZERO);
  TAILQ_INIT(&dev->children);
  SLIST_INIT(&dev->resources);
  dev->unit = unit;
  return dev;
}

device_t *device_add_child(device_t *parent, int unit) {
  device_t *child = device_alloc(unit);
  child->parent = parent;
  TAILQ_INSERT_TAIL(&parent->children, child, link);
  return child;
}

/* TODO: this routine should go over all drivers within a suitable class and
 * choose the best driver. For now the user is responsible for setting the
 * driver before calling `device_probe`. */
int device_probe(device_t *dev) {
  assert(dev->driver != NULL);
  d_probe_t probe = dev->driver->probe;
  int found = probe ? probe(dev) : 0;
  if (found)
    dev->state = kmalloc(M_DEV, dev->driver->size, M_ZERO);
  return found;
}

int device_attach(device_t *dev) {
  assert(dev->driver != NULL);
  d_attach_t attach = dev->driver->attach;
  return attach ? attach(dev) : ENODEV;
}

int device_detach(device_t *dev) {
  assert(dev->driver != NULL);
  d_detach_t detach = dev->driver->detach;
  int res = detach ? detach(dev) : 0;
  if (res == 0)
    kfree(M_DEV, dev->state);
  return res;
}

static resource_t *resource_list_find(device_t *dev, res_type_t type, int rid) {
  resource_t *r;
  SLIST_FOREACH(r, &dev->resources, r_link) {
    if (r->r_type == type && r->r_rid == rid)
      return r;
  }
  return NULL;
}

void device_add_resource(device_t *dev, res_type_t type, int rid,
                         rman_addr_t start, rman_addr_t end, size_t size,
                         res_flags_t flags) {
  assert(!resource_list_find(dev, rid, type));
  resource_t *r = bus_alloc_resource(dev, type, rid, start, end, size, flags);
  SLIST_INSERT_HEAD(&dev->resources, r, r_link);
}

void device_remove_resource(device_t *dev, resource_t *r) {
  assert(resource_list_find(dev, r->r_rid, r->r_type));
  SLIST_REMOVE(&dev->resources, r, resource, r_link);
  bus_release_resource(dev, r);
}

resource_t *device_take_resource(device_t *dev, res_type_t type, int rid,
                                 res_flags_t flags) {
  resource_t *r = resource_list_find(dev, type, rid);
  if (!r)
    return NULL;

  if (flags & RF_ACTIVE)
    bus_activate_resource(dev, r);

  return r;
}
