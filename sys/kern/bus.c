#define KL_LOG KL_DEV
#include <sys/klog.h>
#include <sys/device.h>
#include <sys/pci.h>
#include <sys/kmem.h>
#include <sys/pmap.h>
#include <sys/rman.h>
#include <sys/devclass.h>

static KMALLOC_DEFINE(M_RLE, "resource list entries");

void resource_list_init(device_t *dev) {
  SLIST_INIT(&dev->resources);
}

void resource_list_fini(device_t *dev) {
  resource_list_t *rl = &dev->resources;
  resource_list_entry_t *rle;

  while ((rle = SLIST_FIRST(rl))) {
    if (rle->res)
      panic("resource entry is busy");
    SLIST_REMOVE_HEAD(rl, link);
    kfree(M_RLE, rle);
  }
}

resource_list_entry_t *resource_list_find(device_t *dev, res_type_t type,
                                          int rid) {
  resource_list_entry_t *rle;

  SLIST_FOREACH(rle, &dev->resources, link) {
    if (rle->type == type && rle->rid == rid)
      return rle;
  }
  return NULL;
}

void resource_list_add(device_t *dev, res_type_t type, int rid,
                       rman_addr_t start, size_t count) {
  resource_list_entry_t *rle;

  if ((rle = resource_list_find(dev, rid, type)))
    panic("resource entry already exists");

  rle = kmalloc(M_RLE, sizeof(resource_list_entry_t), M_WAITOK);
  SLIST_INSERT_HEAD(&dev->resources, rle, link);
  rle->res = NULL;
  rle->type = type;
  rle->rid = rid;
  rle->start = start;
  rle->count = count;
}

resource_t *resource_list_alloc(device_t *dev, rman_t *rman, res_type_t type,
                                int rid, res_flags_t flags) {
  resource_list_entry_t *rle;

  if (!(rle = resource_list_find(dev, type, rid))) {
    /* no resource entry of that type/rid */
    return NULL;
  }

  rman_addr_t end = rle->start + rle->count - 1;
  resource_t *r =
    rman_alloc_resource(rman, rle->start, end, rle->count, 1, flags);
  if (r)
    rle->res = r;

  return r;
}

void resource_list_release(device_t *dev, res_type_t type, int rid,
                           resource_t *res) {
  resource_list_entry_t *rle;

  if (!(rle = resource_list_find(dev, type, rid)))
    panic("can't find the resource entry");

  if (!rle->res)
    panic("resource entry is not busy");

  /* TODO: uncomment the following after merging the new rman. */
  /*rman_deactivate_resource(rle->res);*/
  rman_release_resource(rle->res);
  rle->res = NULL;
}

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

int bus_activate_resource(device_t *dev, res_type_t type, int rid,
                          resource_t *r) {
  if (r->r_flags & RF_ACTIVE)
    return 0;

  int error = BUS_DRIVER(dev)->bus.activate_resource(dev, type, rid, r);
  if (error == 0)
    rman_activate_resource(r);
  return error;
}

int bus_generic_probe(device_t *bus) {
  int error = 0;
  devclass_t *dc = bus->devclass;
  if (!dc)
    return error;
  device_t *dev;
  TAILQ_FOREACH (dev, &bus->children, link) {
    driver_t **drv_p;
    DEVCLASS_FOREACH(drv_p, dc) {
      dev->driver = *drv_p;
      if (device_probe(dev)) {
        klog("%s detected!", dev->driver->desc);
        /* device_attach returns error ! */
        if (!device_attach(dev)) {
          klog("%s attached to %p!", dev->driver->desc, dev);
          break;
        }
      }
      dev->driver = NULL;
    }
  }
  return error;
}
