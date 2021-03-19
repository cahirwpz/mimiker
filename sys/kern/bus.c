#define KL_LOG KL_DEV
#include <sys/klog.h>
#include <sys/bus.h>
#include <sys/kmem.h>
#include <sys/pmap.h>
#include <sys/devclass.h>

int generic_bs_map(bus_addr_t addr, bus_size_t size,
                   bus_space_handle_t *handle_p) {
  *handle_p = kmem_map_contig(addr, size, PMAP_NOCACHE);
  return 0;
}

uint8_t generic_bs_read_1(bus_space_handle_t handle, bus_size_t offset) {
  return *(volatile uint8_t *)(handle + offset);
}

uint16_t generic_bs_read_2(bus_space_handle_t handle, bus_size_t offset) {
  return *(volatile uint16_t *)(handle + offset);
}

uint32_t generic_bs_read_4(bus_space_handle_t handle, bus_size_t offset) {
  return *(volatile uint32_t *)(handle + offset);
}

void generic_bs_write_1(bus_space_handle_t handle, bus_size_t offset,
                        uint8_t value) {
  *(volatile uint8_t *)(handle + offset) = value;
}

void generic_bs_write_2(bus_space_handle_t handle, bus_size_t offset,
                        uint16_t value) {
  *(volatile uint16_t *)(handle + offset) = value;
}

void generic_bs_write_4(bus_space_handle_t handle, bus_size_t offset,
                        uint32_t value) {
  *(volatile uint32_t *)(handle + offset) = value;
}

void generic_bs_read_region_1(bus_space_handle_t handle, bus_size_t offset,
                              uint8_t *dst, bus_size_t count) {
  uint8_t *src = (uint8_t *)(handle + offset);
  for (size_t i = 0; i < count; i++)
    *dst++ = *src++;
}

void generic_bs_read_region_2(bus_space_handle_t handle, bus_size_t offset,
                              uint16_t *dst, bus_size_t count) {
  uint16_t *src = (uint16_t *)(handle + offset);
  for (size_t i = 0; i < count; i++)
    *dst++ = *src++;
}

void generic_bs_read_region_4(bus_space_handle_t handle, bus_size_t offset,
                              uint32_t *dst, bus_size_t count) {
  uint32_t *src = (uint32_t *)(handle + offset);
  for (size_t i = 0; i < count; i++)
    *dst++ = *src++;
}

void generic_bs_write_region_1(bus_space_handle_t handle, bus_size_t offset,
                               const uint8_t *src, bus_size_t count) {
  uint8_t *dst = (uint8_t *)(handle + offset);
  for (size_t i = 0; i < count; i++)
    *dst++ = *src++;
}

void generic_bs_write_region_2(bus_space_handle_t handle, bus_size_t offset,
                               const uint16_t *src, bus_size_t count) {
  uint16_t *dst = (uint16_t *)(handle + offset);
  for (size_t i = 0; i < count; i++)
    *dst++ = *src++;
}

void generic_bs_write_region_4(bus_space_handle_t handle, bus_size_t offset,
                               const uint32_t *src, bus_size_t count) {
  uint32_t *dst = (uint32_t *)(handle + offset);
  for (size_t i = 0; i < count; i++)
    *dst++ = *src++;
}

bus_space_t *generic_bus_space = &(bus_space_t){
  .bs_map = generic_bs_map,
  .bs_read_1 = generic_bs_read_1,
  .bs_read_2 = generic_bs_read_2,
  .bs_read_4 = generic_bs_read_4,
  .bs_write_1 = generic_bs_write_1,
  .bs_write_2 = generic_bs_write_2,
  .bs_write_4 = generic_bs_write_4,
  .bs_read_region_1 = generic_bs_read_region_1,
  .bs_read_region_2 = generic_bs_read_region_2,
  .bs_read_region_4 = generic_bs_read_region_4,
  .bs_write_region_1 = generic_bs_write_region_1,
  .bs_write_region_2 = generic_bs_write_region_2,
  .bs_write_region_4 = generic_bs_write_region_4,
};

void bus_intr_setup(device_t *dev, resource_t *irq, ih_filter_t *filter,
                    ih_service_t *service, void *arg, const char *name) {
  device_t *idev = BUS_METHOD_PROVIDER(dev, intr_setup);
  bus_methods(idev->parent)->intr_setup(idev, irq, filter, service, arg, name);
  if (irq->r_handler)
    resource_activate(irq);
}

void bus_intr_teardown(device_t *dev, resource_t *irq) {
  assert(resource_active(irq));
  device_t *idev = BUS_METHOD_PROVIDER(dev, intr_teardown);
  bus_methods(idev->parent)->intr_teardown(idev, irq);
  irq->r_handler = NULL;
  resource_deactivate(irq);
}

int bus_activate_resource(device_t *dev, resource_t *r) {
  if (resource_active(r) || r->r_type == RT_IRQ)
    return 0;

  device_t *idev = BUS_METHOD_PROVIDER(dev, activate_resource);
  int error = bus_methods(idev->parent)->activate_resource(idev, r);
  if (error == 0)
    resource_activate(r);
  return error;
}

void bus_deactivate_resource(device_t *dev, resource_t *r) {
  if (r->r_type != RT_IRQ) {
    assert(resource_active(r));
    device_t *idev = BUS_METHOD_PROVIDER(dev, deactivate_resource);
    bus_methods(idev->parent)->deactivate_resource(idev, r);
    resource_deactivate(r);
  }
}

/* System-wide current pass number. */
static drv_pass_t current_pass;

int bus_generic_probe(device_t *bus) {
  devclass_t *dc = bus->devclass;
  if (!dc)
    return 0;
  device_t *dev;
  TAILQ_FOREACH (dev, &bus->children, link) {
    if (dev->driver) {
      if (device_bus(dev))
        bus_generic_probe(dev);
      continue;
    }

    driver_t **drv_p;
    DEVCLASS_FOREACH(drv_p, dc) {
      driver_t *driver = *drv_p;
      if (driver->pass > current_pass)
        continue;
      dev->driver = driver;
      if (device_probe(dev)) {
        klog("%s detected!", driver->desc);
        /* device_attach returns error ! */
        if (!device_attach(dev)) {
          klog("%s attached to %p!", driver->desc, dev);
          break;
        }
      }
      dev->driver = NULL;
    }
  }
  return 0;
}

DEVCLASS_DECLARE(root);

void init_devices(void) {
  assert(current_pass < PASS_COUNT);
  extern driver_t rootdev_driver;
  static device_t *rootdev;
  if (rootdev == NULL) {
    rootdev = device_alloc(0);
    rootdev->devclass = &DEVCLASS(root);
    rootdev->driver = (driver_t *)&rootdev_driver;
    device_probe(rootdev);
    device_attach(rootdev);
  } else {
    bus_generic_probe(rootdev);
  }
  current_pass++;
}
