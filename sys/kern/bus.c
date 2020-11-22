#define KL_LOG KL_DEV
#include <sys/klog.h>
#include <sys/device.h>
#include <sys/pci.h>
#include <sys/kmem.h>
#include <sys/pmap.h>
#include <sys/rman.h>
#include <sys/devclass.h>

static KMALLOC_DEFINE(M_RLE, "resource list entries");

void resource_list_init(resource_list_t *rl) {
  SLIST_INIT(rl);
}

void resource_list_fini(resource_list_t *rl) {
  resource_list_entry_t *rle;

  while ((rle = SLIST_FIRST(rl))) {
    if (!SLIST_EMPTY(&rle->resources))
      panic("resource entry contains allocated resources");
    SLIST_REMOVE_HEAD(rl, link);
    kfree(M_RLE, rle);
  }
}

resource_list_entry_t *resource_list_find(resource_list_t *rl, res_type_t type,
                                          int rid) {
  resource_list_entry_t *rle;

  SLIST_FOREACH(rle, rl, link)
  if (rle->type == type && rle->rid == rid)
    return rle;
  return NULL;
}

void resource_list_add(resource_list_t *rl, res_type_t type, int rid,
                       rman_addr_t start, rman_addr_t end, size_t count) {
  resource_list_entry_t *rle;

  if ((rle = resource_list_find(rl, rid, type)))
    panic("resource entry already exists");

  rle = kmalloc(M_RLE, sizeof(resource_list_entry_t), M_WAITOK);
  assert(rle);
  SLIST_INSERT_HEAD(rl, rle, link);
  SLIST_INIT(&rle->resources);
  rle->type = type;
  rle->rid = rid;
  rle->start = start;
  rle->end = end;
  rle->count = count;
}

resource_t *resource_list_alloc(resource_list_t *rl, rman_t *rman,
                                res_type_t type, int rid, rman_addr_t start,
                                rman_addr_t end, size_t count,
                                res_flags_t flags) {
  resource_list_entry_t *rle;
  bool isdefault = RMAN_IS_DEFAULT(start, end, count);
  assert(rl);

  if (!(rle = resource_list_find(rl, type, rid))) {
    /* no resource entry of that type/rid */
    return NULL;
  }

  if (isdefault) {
    start = rle->start;
    end = rle->end;
    count = rle->count;
  }

  resource_t *r = rman_alloc_resource(rman, start, end, count, 1, flags);
  if (r)
    SLIST_INSERT_HEAD(&rle->resources, r, r_rle_link);

  return r;
}

void resource_list_release(resource_list_t *rl, res_type_t type, int rid,
                           resource_t *res) {
  resource_list_entry_t *rle;
  assert(rl);

  if (!(rle = resource_list_find(rl, type, rid)))
    panic("can't find the resource entry");

  resource_t *r;
  SLIST_FOREACH(r, &rle->resources, r_rle_link)
  if (r == res)
    break;

  if (!r)
    panic("can't find the resource within the resource entry");

  SLIST_REMOVE(rl, rle, resource_list_entry, link);
  rman_release_resource(r);
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
