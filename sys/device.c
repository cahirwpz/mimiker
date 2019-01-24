#define KL_LOG KL_DEV
#include <stdc.h>
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

device_t *make_device(device_t *parent, driver_t *driver, char *name,
                      int unit) {
  assert(strlen(name) < MAX_DEV_NAME_LEN);
  device_t *dev = device_add_child(parent);
  dev->driver = driver;
  dev->name = name;
  dev->unit = unit;
  if (device_probe(dev))
    device_attach(dev);
  return dev;
}

void device_add_resource(device_t *dev, resource_t *r, int rid) {
  r->r_owner = dev;
  r->r_id = rid;
  TAILQ_INSERT_HEAD(&dev->resources, r, r_device);
}

int device_get_fullname(device_t *dev, char *buff, int size) {
  assert(dev != NULL);
  return snprintf(buff, size, "%s@%d", dev->name, dev->unit);
}

static void _construct_fullpath_aux(device_t *dev, char *buff, size_t size) {
  if (dev == NULL || !strcmp(dev->name, "root"))
    return;
  char dev_name_buff[MAX_DEV_NAME_LEN];
  device_get_fullname(dev, dev_name_buff, MAX_DEV_NAME_LEN);

  char tmp_buff[PATHBUF_LEN];
  snprintf(tmp_buff, size, "/%s%s", dev_name_buff, buff);
  snprintf(buff, size, "%s", tmp_buff);
  _construct_fullpath_aux(dev->parent, buff, size);
}

void device_construct_fullpath(device_t *dev, char *buff, size_t size) {
  buff[0] = '\0';
  _construct_fullpath_aux(dev, buff, size);
}
