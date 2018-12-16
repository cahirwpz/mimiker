#define KL_LOG KL_DEV
#include <klog.h>
#include <device.h>
#include <pci.h>
#include <rman.h>

int generic_bs_map(bus_addr_t addr, bus_size_t size, int flags,
                   bus_space_handle_t *handle_p) {
  *handle_p = addr;
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

int bus_generic_probe(device_t *bus) {
  device_t *dev;
  SET_DECLARE(driver_table, driver_t);
  klog("Scanning %s for known devices.", bus->driver->desc);
  TAILQ_FOREACH (dev, &bus->children, link) {
    driver_t **drv_p;
    SET_FOREACH (drv_p, driver_table) {
      driver_t *drv = *drv_p;
      dev->driver = drv;
      if (device_probe(dev)) {
        klog("%s detected!", drv->desc);
        device_attach(dev);
        break;
      }
      dev->driver = NULL;
    }
  }
  return 0;
}

/*
typedef struct {
        char* path;
        uint32_t iomem[32];
        uint32_t ioport[32];
        uint32_t irq;
} devhint_t;

devhint_t hints[] = {
        .irq = 0x4,
        .path = "/rootdev/pci@0/isab@0/isa@0/uart@0",
        .iomem = {1016, 1023, 760, 767}
},
{
        .irq = 0x3,
        .ioport = {96, 96, 100, 100},
        .path = "/rootdev/pci@0/isab@0/isa@0/uart@1",
        .iomem = {760, 767}
},
        };
*/

// identify hinted devices and resources.
// add devices to parentbus
// add hinted resources to devices ivars
//
/*
int bus_enumerate_hinted_children(device_t *bus){

        //

}*/

/* EXAMPLE BUS_ATTACH
 *
 * (...)
 * bus_specific_identify_identifiable_children_and_resources(bus_dev);
 *
 * busdriver is responsible for storing child devivce's resources (identifiable
 * and non-identifiable) in device's resources. Child device in order to
 * interact with resource in any way must use interface provided by parentbus
 *
 * bus_enumerate_hinted_children(bus_dev);
 * now all child devices are identified and created but with no drivers assigned
 * bus_generic_probe_and_attach2(bus_dev);
 *
 * check for errors here
 *
 *
 */

int __attribute__((unused)) bus_generic_attach(device_t *dev) {
  device_t *child;

  TAILQ_FOREACH (child, &dev->children, link) {
    int error = device_probe_and_attach(child);
    if(error > 0){
      klog("pribe & attach failed for [device_name]!\n");
    }
  }

  return 0;
}
