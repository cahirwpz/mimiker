#define KL_LOG KL_DEV
#include <sys/klog.h>
#include <sys/device.h>
#include <sys/pci.h>
#include <sys/kmem.h>
#include <sys/pmap.h>
#include <sys/rman.h>
#include <sys/devclass.h>

int generic_bs_map(bus_addr_t addr, bus_size_t size,
                   bus_space_handle_t *handle_p) {
  vaddr_t handle = kva_alloc(size);
  for (bus_size_t start = 0; start < size; start += PAGESIZE) {
    pmap_kenter(handle + start, addr + start, VM_PROT_READ | VM_PROT_WRITE,
                PMAP_NOCACHE);
  }
  *handle_p = handle;
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

#define DEV_IDENTIFIED_BY_PARENT ((void *)0xffffffff)

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

static int bus_try_attach(device_t *dev, driver_t *drv) {
  dev->driver = drv;
  if (device_probe(dev)) {
    klog("%s detected!", drv->desc);
    return device_attach(dev);
  }
  return 0;
}

int bus_generic_probe(device_t *bus) {
  int error = 0;
  devclass_t *dc = bus->devclass;
  if (!dc)
    return error;
  driver_t **drv_p;
  DEVCLASS_FOREACH(drv_p, dc) {
    driver_t *drv = *drv_p;
    device_t *dev = device_identify(drv, bus);
    if (dev == NULL)
      continue;
    if (dev == DEV_IDENTIFIED_BY_PARENT) {
      device_t *child_dev;
      TAILQ_FOREACH (child_dev, &bus->children, link) {
        if ((error = bus_try_attach(child_dev, drv)))
          return error;
      }
    } else {
      if ((error = bus_try_attach(dev, drv)))
        return error;
    }
  }
  return error;
}

device_t *bus_generic_identify(driver_t *driver, device_t *bus) {
  return DEV_IDENTIFIED_BY_PARENT;
}