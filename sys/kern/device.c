#define KL_LOG KL_DEV
#include <sys/klog.h>
#include <sys/errno.h>
#include <sys/mimiker.h>
#include <sys/device.h>
#include <sys/rman.h>

KMALLOC_DEFINE(M_DEV, "devices & drivers");

typedef struct resource_list_entry {
  SLIST_ENTRY(resource_list_entry) link;
  resource_t *res;   /* the actual resource when allocated */
  res_type_t type;   /* type argument to alloc_resource */
  int rid;           /* resource identifier */
  rman_addr_t start; /* start of resource range */
  rman_addr_t end;   /* end of resource range */
  size_t count;      /* number of bytes */
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
    if (rle->type == type && rle->rid == rid)
      return rle;
  }
  return NULL;
}

void device_add_resource(device_t *dev, res_type_t type, int rid,
                         rman_addr_t start, rman_addr_t end, size_t count) {
  resource_list_entry_t *rle = resource_list_find(dev, rid, type);
  assert(rle == NULL);

  rle = kmalloc(M_DEV, sizeof(resource_list_entry_t), M_WAITOK);
  rle->res = NULL;
  rle->type = type;
  rle->rid = rid;
  rle->start = start;
  rle->end = end;
  rle->count = count;
  SLIST_INSERT_HEAD(&dev->resources, rle, link);
}

resource_t *device_alloc_resource(device_t *dev, rman_t *rman, res_type_t type,
                                  int rid, res_flags_t flags) {
  resource_list_entry_t *rle = resource_list_find(dev, type, rid);
  if (rle == NULL)
    return NULL;

  size_t alignment = (type == RT_MEMORY) ? PAGESIZE : 0;

  resource_t *r = rman_reserve_resource(rman, rle->start, rle->end, rle->count,
                                        alignment, flags);
  if (r == NULL)
    return NULL;

  rle->res = r;
  return r;
}

void device_release_resource(device_t *dev, res_type_t type, int rid,
                             resource_t *res) {
  resource_list_entry_t *rle = resource_list_find(dev, type, rid);
  assert(rle);      /* can't find the resource entry */
  assert(rle->res); /* resource entry is not busy */

  rman_deactivate_resource(rle->res);
  rman_release_resource(rle->res);
  rle->res = NULL;
}
