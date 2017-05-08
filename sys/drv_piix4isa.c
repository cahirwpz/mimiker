#include <device.h>
#include <pci.h>
#include <isa.h>
#include <stdc.h>

#define PIIX4ISA_VENDOR_ID 0x8086
#define PIIX4ISA_DEVICE_ID 0x7110

typedef struct piix4isa_state {
  resource_t* ioports;
  /* This resource represents the ISA bus behind this PCI-ISA bridge. The
     resource is owned by the device. */
  resource_t isa;
} piix4isa_state_t;

static uint8_t piix4_isa_read1(resource_t* handle, unsigned offset) {
  device_t* isa_bus = handle->r_owner;
  piix4isa_state_t* bus_state = isa_bus->state;
  return bus_space_read_1(bus_state->ioports, offset);
}
static void piix4_isa_write1(resource_t* handle, unsigned offset, uint8_t val) {
  device_t* isa_bus = handle->r_owner;
  piix4isa_state_t* bus_state = isa_bus->state;
  bus_space_write_1(bus_state->ioports, offset, val);
}

static bus_space_t piix4_isa_bus_space = {
  .read_1 = piix4_isa_read1,
  .write_1 = piix4_isa_write1
};

static int piix4isa_probe(device_t* dev){
  pci_device_t *pcid = pci_device_of(dev);
  if(!pcid)
    return 0;

  if(pcid->vendor_id != PIIX4ISA_VENDOR_ID ||
     pcid->device_id != PIIX4ISA_DEVICE_ID)
    return 0;

  return 1;
}

static int piix4isa_attach(device_t* dev){
  assert(dev->bus == DEV_BUS_PCI);
  pci_bus_state_t *pci_bus_data = dev->parent->state;

  piix4isa_state_t* piix4isa_bus = dev->state;
  /* TODO: bus_allocate_resource */
  piix4isa_bus->ioports = pci_bus_data->lowio_space;

  piix4isa_bus->isa = (resource_t){
    .r_owner = dev,
    .r_bus_space = &piix4_isa_bus_space,
    .r_type = RT_IOPORTS,
    .r_start = 0,
    .r_end = ISA_BUS_IOPORTS_SIZE - 1,
  };

  driver_t **drv_p;
  SET_DECLARE(driver_table, driver_t);
  SET_FOREACH(drv_p, driver_table){
    driver_t* drv = *drv_p;
    /* Here a new temporary device is created, a potential child of the ISA
       bus. */
    device_t* chdev = device_add_child(dev);
    chdev->bus = DEV_BUS_ISA;
    isa_device_t* isa_dev = kmalloc(M_DEV, sizeof(isa_device_t), M_ZERO);
    
    /* TODO: bus_allocate_resource */
    /* Allocate entire ioports space for the child device. */
    isa_dev->isa_bus = &piix4isa_bus->isa;
    
    chdev->instance = isa_dev;
    chdev->driver = drv;
    if(device_probe(chdev)) {
      log("%s detected on PIIX4 PCI-ISA bridge!", drv->desc);
      device_attach(chdev);
      break;
    }
    /* TODO: Throw away chdev, remove it from dev's children. */
  }
  
  return 0;
}

static driver_t piix4isa = {
  .desc = "PIIX4 PCI-ISA bridge driver",
  .size = sizeof(piix4isa_state_t),
  .probe = piix4isa_probe,
  .attach = piix4isa_attach,
};
DRIVER_ADD(piix4isa);
