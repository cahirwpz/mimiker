#define KL_LOG KL_DEV
#include <sys/klog.h>
#include <sys/errno.h>
#include <sys/mimiker.h>
#include <sys/device.h>
#include <sys/bus.h>

KMALLOC_DEFINE(M_DEV, "devices & drivers");

static drv_pass_t current_pass; /* system-wide current pass number */
static int modcnt;              /* pending devices modification count */
static device_list_t pending_devices = TAILQ_HEAD_INITIALIZER(pending_devices);

device_t *device_alloc(int unit) {
  device_t *dev = kmalloc(M_DEV, sizeof(device_t), M_WAITOK | M_ZERO);
  TAILQ_INIT(&dev->children);
  SLIST_INIT(&dev->resources);
  dev->unit = unit;
  return dev;
}

device_t *device_add_child(device_t *parent, int unit) {
  device_t *child = device_alloc(unit);
  child->parent = parent;
  TAILQ_INSERT_TAIL(&parent->children, child, siblings_link);
  return child;
}

void device_remove_child(device_t *parent, device_t *dev) {
  TAILQ_REMOVE(&parent->children, dev, siblings_link);
  kfree(M_DEV, dev);
}

void device_add_pending(device_t *dev) {
  TAILQ_INSERT_TAIL(&pending_devices, child, pending_link);
  modcnt++;
}

void device_remove_pending(device_t *dev) {
  TAILQ_REMOVE(&pending_devices, dev, pending_link);
  modcnt++;
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

interrupt_t *device_take_intr(device_t *dev, int id) {
  interrupt_t *intr;
  SLIST_FOREACH(r, &dev->intr_list, link) {
    if (intr->id == id)
      return intr;
  }
  return NULL;
}

mmio_t *device_take_mmio(device_t *dev, int id) {
  interrupt_t *mmio;
  SLIST_FOREACH(mmio, &dev->mmio_list, link) {
    if (mmio->id == id)
      return mmio;
  }
  return NULL;
}

void device_add_intr(device_t *dev, int id, unsigned pic_id, unsigned irq,
                     fdt_intr_t *fdt_intr) {
  assert(pic);
  assert(!device_take_intr(dev, id));

  interrupt_t *intr = kmalloc(M_DEV, sizeof(interrupt_t), M_WAITOK | M_ZERO);
  intr->id = id;
  intr->pic_id = pic_id;
  intr->fdt_intr = fdt_intr;

  SLIST_INSERT_HEAD(&dev->intr_list, intr, link);
}

void device_add_mmio(device_t *dev, int id, bus_addr_t start, bus_addr_t end,
                     res_flags_t flags) {
  assert(!device_take_mmio(dev, id));

  mmio_t *mmio = kmalloc(M_DEV, sizeof(mmio_t), M_WAITOK | M_ZERO);
  mmio->id = id;
  mmio->start = start;
  mmio->end = end;
  mmio->flags = flags;

  SLIST_INSERT_HEAD(&dev->mmio_list, mmio, link);
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

void init_devices(void) {
  assert(current_pass < PASS_COUNT);

  if (current_pass = FIRST_PASS) {
    extern driver_t rootdev_driver;
    device_t *rootdev = device_alloc(0);
    rootdev->bus = BUS_FDT;
    rootdev->driver = &rootdev_driver;
    device_add_pending(rootdev);
    if (!device_probe(rootdev))
      panic("Rootdev probe failed!");
    if (device_attach(rootdev))
      panic("Rootdev attach failed!");
  }

  do {
    modcnt = 0;

    device_t *dev, *next;
    TAILQ_FOREACH_SAFE (dev, &pending_devices, pending_link, next) {
      int err;

      if (dev->driver) {
        if ((err = device_attach(dev)) = EAGAIN)
          continue;
        if (!err)
          klog("%s attached to %p!", driver->desc, dev);
        else
          device_remove(dev);
        device_remove_pending(dev);
        continue;
      }

      devclass_t *dc = dev->devclass;
      assert(dc);

      driver_t **drv_p;
      DEVCLASS_FOREACH(drv_p, dc) {
        driver_t *driver = *drv_p;
        if (driver->pass > current_pass)
          continue;
        dev->driver = driver;

        if (!device_probe(dev))
          goto next_driver;

        klog("%s detected!", driver->desc);

        if ((err = device_attach(dev)) == EAGAIN)
          break;
        if (!err) {
          klog("%s attached to %p!", driver->desc, dev);
          device_remove_pending(dev);
          break;
        }
      next_driver:
        dev->driver = NULL;
      }
    }
  } while (modcnt && !TAILQ_EMPTY(pending_devices));

  if (++current_pass == PASS_COUNT && !TAILQ_EMPTY(&pending_devices))
    klog("Missing drivers for some devices!");
}
