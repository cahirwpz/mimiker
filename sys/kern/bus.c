#define KL_LOG KL_DEV
#include <sys/klog.h>
#include <sys/bus.h>
#include <sys/kmem.h>
#include <sys/pmap.h>
#include <sys/devclass.h>

int generic_bs_map(bus_addr_t addr, bus_size_t size,
                   bus_space_handle_t *handle_p) {
  bus_addr_t pg_start = rounddown2(addr, PAGESIZE);
  bus_addr_t pg_end = roundup2(addr + size, PAGESIZE);
  bus_size_t off = addr & (PAGESIZE - 1);

  vaddr_t va = kmem_map_contig(pg_start, pg_end - pg_start, PMAP_NOCACHE);
  *handle_p = va + off;
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

int bus_map_mmio(device_t *dev, mmio_t *mmio) {
  if (mmio->bus_handle)
    return 0;

  device_t *idev = BUS_METHOD_PROVIDER(dev, map_mmio);
  return bus_methods(idev->parent)->map_mmio(idev, mmio);
}

void bus_unmap_mmio(device_t *dev, mmio_t *mmio) {
  assert(mmio->bus_handle);
  device_t *idev = BUS_METHOD_PROVIDER(dev, unmap_mmio);
  bus_methods(idev->parent)->unmap_mmio(idev, mmio);
  mmio->bus_handle = 0;
}
