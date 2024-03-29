#define KL_LOG KL_DEV
#include <sys/klog.h>
#include <sys/errno.h>
#include <sys/mimiker.h>
#include <sys/device.h>
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

void device_remove_child(device_t *parent, device_t *dev) {
  TAILQ_REMOVE(&parent->children, dev, link);
  kfree(M_DEV, dev);
}

/* TODO: this routine should go over all drivers within a suitable class and
 * choose the best driver. For now the user is responsible for setting the
 * driver before calling `device_probe`. */
int device_probe(device_t *dev) {
  assert(dev->driver != NULL);
  d_probe_t probe = dev->driver->probe;
  return probe ? probe(dev) : 0;
}

int device_attach(device_t *dev) {
  assert(dev->driver != NULL);
  d_attach_t attach = dev->driver->attach;
  if (attach == NULL)
    return ENODEV;
  dev->state = kmalloc(M_DEV, dev->driver->size, M_ZERO);
  int err = attach(dev);
  if (!err)
    return 0;
  kfree(M_DEV, dev->state);
  return err;
}

int device_detach(device_t *dev) {
  assert(dev->driver != NULL);
  d_detach_t detach = dev->driver->detach;
  int res = detach ? detach(dev) : 0;
  if (res == 0)
    kfree(M_DEV, dev->state);
  return res;
}

static resource_t *resource_alloc(res_type_t type, int rid, res_flags_t flags) {
  resource_t *r = kmalloc(M_DEV, sizeof(resource_t), M_WAITOK | M_ZERO);
  r->r_type = type;
  r->r_rid = rid;
  r->r_flags = flags;
  return r;
}

resource_t *device_take_resource(device_t *dev, res_type_t type, int rid) {
  resource_t *r;
  SLIST_FOREACH(r, &dev->resources, r_link) {
    if (r->r_type == type && r->r_rid == rid)
      return r;
  }
  return NULL;
}

void device_add_irq(device_t *dev, int rid, unsigned irq) {
  assert(!device_take_resource(dev, rid, RT_IRQ));

  resource_t *r = resource_alloc(RT_IRQ, rid, 0);
  r->r_irq = irq;

  SLIST_INSERT_HEAD(&dev->resources, r, r_link);
}

void device_add_range(device_t *dev, res_type_t type, int rid, bus_addr_t start,
                      bus_addr_t end, res_flags_t flags) {
  assert(type != RT_IRQ);
  assert(!(type == RT_IOPORTS && (flags & RF_PREFETCHABLE)));
  assert(!device_take_resource(dev, rid, type));

  resource_t *r = resource_alloc(type, rid, flags);
  r->r_start = start;
  r->r_end = end;

  SLIST_INSERT_HEAD(&dev->resources, r, r_link);
}

device_t *device_method_provider(device_t *dev, drv_if_t iface,
                                 ptrdiff_t method_offset) {
  for (; dev->parent; dev = dev->parent) {
    void *interface = dev->parent->driver->interfaces[iface];
    if (interface && *(void **)(interface + method_offset))
      return dev;
  }

  panic("Device has no parent!");
}
