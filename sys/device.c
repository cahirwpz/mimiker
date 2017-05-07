#include <device.h>

MALLOC_DEFINE(M_DEV, "device/driver pool", 128, 1024);

static device_t *device_alloc() {
  device_t *dev = kmalloc(M_DEV, sizeof(device_t), M_ZERO);
  TAILQ_INIT(&dev->children);
  return dev;
}

int device_set_driver(device_t *dev, driver_t *driver) {
  dev->driver = driver;
  dev->state = kmalloc(M_DEV, driver->size, M_ZERO);
  return 0;
}

device_t *device_add_child(device_t *dev) {
  device_t *child = device_alloc();
  child->parent = dev;
  TAILQ_INSERT_TAIL(&dev->children, child, link);
  return child;
}

int device_attach(device_t *dev) {
  return dev->driver->attach(dev);
}
