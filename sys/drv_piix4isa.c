#include <device.h>
#include <pci.h>
#include <isa.h>
#include <stdc.h>

#define PIIX4_ISA_VENDOR_ID 0x8086
#define PIIX4_ISA_DEVICE_ID 0x7110

typedef struct piix4_isa_state {
  resource_t *ioports;
  /* This resource represents the ISA bus behind this PCI-ISA bridge. The
     resource is owned by the device. */
  resource_t isa;
} piix4_isa_state_t;

static uint8_t piix4_isa_read_1(resource_t *handle, unsigned offset) {
  device_t *isa_bus = handle->r_owner;
  piix4_isa_state_t *bus_state = isa_bus->state;
  return bus_space_read_1(bus_state->ioports, offset);
}
static void piix4_isa_write_1(resource_t *handle, unsigned offset,
                              uint8_t val) {
  device_t *isa_bus = handle->r_owner;
  piix4_isa_state_t *bus_state = isa_bus->state;
  bus_space_write_1(bus_state->ioports, offset, val);
}
static uint16_t piix4_isa_read_2(resource_t *handle, unsigned offset) {
  device_t *isa_bus = handle->r_owner;
  piix4_isa_state_t *bus_state = isa_bus->state;
  return bus_space_read_2(bus_state->ioports, offset);
}
static void piix4_isa_write_2(resource_t *handle, unsigned offset,
                              uint16_t val) {
  device_t *isa_bus = handle->r_owner;
  piix4_isa_state_t *bus_state = isa_bus->state;
  bus_space_write_2(bus_state->ioports, offset, val);
}
/* NOTE: Region access to ISA bus is highly unusual. */
static void piix4_isa_read_region_1(resource_t *handle, unsigned offset,
                                    uint8_t *dst, size_t count) {
  device_t *isa_bus = handle->r_owner;
  piix4_isa_state_t *bus_state = isa_bus->state;
  bus_space_read_region_1(bus_state->ioports, offset, dst, count);
}
static void piix4_isa_write_region_1(resource_t *handle, unsigned offset,
                                     const uint8_t *src, size_t count) {
  device_t *isa_bus = handle->r_owner;
  piix4_isa_state_t *bus_state = isa_bus->state;
  bus_space_write_region_1(bus_state->ioports, offset, src, count);
}
static bus_space_t piix4_isa_bus_space = {
  .read_1 = piix4_isa_read_1,
  .write_1 = piix4_isa_write_1,
  .read_2 = piix4_isa_read_2,
  .write_2 = piix4_isa_write_2,
  .read_region_1 = piix4_isa_read_region_1,
  .write_region_1 = piix4_isa_write_region_1};

static int piix4_isa_probe(device_t *dev) {
  pci_device_t *pcid = pci_device_of(dev);
  if (!pcid)
    return 0;

  if (pcid->vendor_id != PIIX4_ISA_VENDOR_ID ||
      pcid->device_id != PIIX4_ISA_DEVICE_ID)
    return 0;

  return 1;
}

static int piix4_isa_attach(device_t *dev) {
  assert(dev->bus == DEV_BUS_PCI);
  pci_bus_state_t *pci_bus_data = dev->parent->state;

  piix4_isa_state_t *piix4_isa_bus = dev->state;
  /* TODO: bus_allocate_resource */
  piix4_isa_bus->ioports = pci_bus_data->lowio_space;

  piix4_isa_bus->isa = (resource_t){
    .r_owner = dev,
    .r_bus_space = &piix4_isa_bus_space,
    .r_type = RT_IOPORTS,
    .r_start = 0,
    .r_end = ISA_BUS_IOPORTS_SIZE - 1,
  };

  driver_t **drv_p;
  SET_DECLARE(driver_table, driver_t);
  SET_FOREACH(drv_p, driver_table) {
    driver_t *drv = *drv_p;
    /* Here a new temporary device is created, a potential child of the ISA
       bus. */
    device_t *chdev = device_add_child(dev);
    chdev->bus = DEV_BUS_ISA;
    isa_device_t *isa_dev = kmalloc(M_DEV, sizeof(isa_device_t), M_ZERO);

    /* TODO: bus_allocate_resource */
    /* Allocate entire ioports space for the child device. */
    isa_dev->isa_bus = &piix4_isa_bus->isa;

    chdev->instance = isa_dev;
    chdev->driver = drv;
    if (device_probe(chdev)) {
      log("%s detected on PIIX4 PCI-ISA bridge!", drv->desc);
      device_attach(chdev);
      break;
    }
    /* TODO: Throw away chdev, remove it from dev's children. */
  }

  return 0;
}

static driver_t piix4_isa = {
  .desc = "PIIX4 PCI-ISA bridge driver",
  .size = sizeof(piix4_isa_state_t),
  .probe = piix4_isa_probe,
  .attach = piix4_isa_attach,
};
DRIVER_ADD(piix4_isa);
