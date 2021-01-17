#define KL_LOG KL_DEV
#include <sys/klog.h>
#include <sys/bus.h>
#include <sys/kmem.h>
#include <sys/pmap.h>
#include <sys/devclass.h>

int generic_bs_map(bus_addr_t addr, bus_size_t size,
                   bus_space_handle_t *handle_p) {
  *handle_p = kmem_map(addr, size, PMAP_NOCACHE);
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

/* clang-format off */
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
/* clang-format on */

int bus_activate_resource(device_t *dev, res_type_t type, resource_t *r) {
  if (r->r_flags & RF_ACTIVE)
    return 0;

  device_t *idev = BUS_METHOD_PROVIDER(dev, activate_resource);
  int error = BUS_METHODS(idev->parent).activate_resource(idev, type, r);
  if (error == 0)
    rman_activate_resource(r);
  return error;
}

void bus_deactivate_resource(device_t *dev, res_type_t type, resource_t *r) {
  if (r->r_flags & RF_ACTIVE) {
    device_t *idev = BUS_METHOD_PROVIDER(dev, deactivate_resource);
    BUS_METHODS(idev->parent).deactivate_resource(idev, type, r);
  }
  rman_deactivate_resource(r);
}

/* System-wide current pass number. */
static pass_num_t current_pass;

int bus_generic_probe(device_t *bus) {
  devclass_t *dc = bus->devclass;
  if (!dc)
    return 0;
  device_t *dev;
  TAILQ_FOREACH (dev, &bus->children, link) {
    driver_t **drv_p;
    DEVCLASS_FOREACH(drv_p, dc) {
      driver_t *driver = *drv_p;
      if (driver->pass != current_pass)
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
    if (device_bus(dev)) {
      /*
       * Bus attach function calls `bus_generic_probe`, but if
       * the current pass number is different than the bus's pass number
       * or the bus doesn't have a driver attached, then the attach function
       * hasn't been called and we need to call `bus_generic_probe` directly.
       */
      driver_t *driver = dev->driver;
      if (!driver || driver->pass != current_pass)
        bus_generic_probe(dev);
    }
  }
  return 0;
}

DEVCLASS_DECLARE(root);
DEVCLASS_CREATE(init);

void init_devices(pass_num_t pass) {
  static device_t *rootdev;
  current_pass = pass;
  if (rootdev == NULL) {
    device_t *primeval = device_alloc(0);
    primeval->devclass = &DEVCLASS(init);
    rootdev = device_alloc(0);
    rootdev->devclass = &DEVCLASS(root);
    TAILQ_INSERT_TAIL(&primeval->children, rootdev, link);
    bus_generic_probe(primeval);
    device_free(primeval);
  } else {
    bus_generic_probe(rootdev);
  }
}
