#define KL_LOG KL_DEV
#include <sys/klog.h>
#include <sys/errno.h>
#include <sys/mimiker.h>
#include <sys/device.h>
#include <sys/rman.h>
#include <sys/bus.h>

KMALLOC_DEFINE(M_DEV, "devices & drivers");

typedef struct resource_list_entry {
  SLIST_ENTRY(resource_list_entry) link;
  resource_t *res; /* the actual resource */
  res_type_t type; /* resource type */
} resource_list_entry_t;

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

static resource_list_entry_t *resource_list_find(device_t *dev, res_type_t type,
                                                 int rid) {
  resource_list_entry_t *rle;
  SLIST_FOREACH(rle, &dev->resources, link) {
    if (rle->type == type && rle->res->r_rid == rid)
      return rle;
  }
  return NULL;
}

void device_add_resource(device_t *dev, res_type_t type, int rid,
                         rman_addr_t start, rman_addr_t end, size_t size,
                         res_flags_t flags) {
  assert(!resource_list_find(dev, rid, type));

  resource_list_entry_t *rle =
    kmalloc(M_DEV, sizeof(resource_list_entry_t), M_WAITOK);
  rle->type = type;
  /* Allocate the actual resource from the parent bus. */
  rle->res = bus_alloc_resource(dev, type, rid, start, end, size, flags);
  assert(rle->res);
  SLIST_INSERT_HEAD(&dev->resources, rle, link);
}

resource_t *device_take_resource(device_t *dev, res_type_t type, int rid,
                                 res_flags_t flags) {
  resource_list_entry_t *rle = resource_list_find(dev, type, rid);
  if (!rle)
    return NULL;

  if (flags & RF_ACTIVE)
    bus_activate_resource(dev, rle->type, rle->res);

  return rle->res;
}

device_t *device_if_find_impl(device_t *dev, size_t iface,
                              size_t method_offset) {
  while ((!dev->driver->interfaces[iface]) ||
         (!*(vaddr_t **)(dev->driver->interfaces[iface] + method_offset))) {
    assert(dev->parent);
    dev = dev->parent;
  }
  return dev;
}
