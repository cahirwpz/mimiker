/* 82371AB PCI-ISA bridge driver */

#include <sys/isa.h>
#include <sys/device.h>
#include <sys/rman.h>
#include <sys/bus.h>
#include <mips/malta.h>
#include <sys/pci.h>
#include <sys/interrupt.h>
#include <dev/piixreg.h>
#include <sys/devclass.h>

#define IO_ISASIZE 1024

#define ICU_ADDR(x) ((x) + 0)
#define ICU_DATA(x) ((x) + 1)
#define ICU1_ADDR ICU_ADDR(IO_ICU1)
#define ICU1_DATA ICU_DATA(IO_ICU1)
#define ICU2_ADDR ICU_ADDR(IO_ICU2)
#define ICU2_DATA ICU_DATA(IO_ICU2)

bus_driver_t intel_isa_bus;

DEVCLASS_DECLARE(isa);

typedef struct intel_isa_state {
  resource_t *io;
  rman_t io_rman;
  intr_event_t *intr_event[ICU_LEN];
} intel_isa_state_t;

static resource_t *intel_isa_alloc_resource(device_t *dev, res_type_t type,
                                            int rid, rman_addr_t start,
                                            rman_addr_t end, size_t size,
                                            res_flags_t flags) {
  rman_t *from = NULL;
  intel_isa_state_t *isa = dev->parent->state;

  if (type == RT_IRQ) {
    /* Just let the PCI driver handle it since it currently holds rman for
     * all the interrupts and we can't have a separate one for ISA
     * unless we have rmans that manage multiple regions */
    device_t *pcib = dev->parent->parent;
    assert(pcib->bus == DEV_BUS_PCI);
    bus_driver_t *pci = pcib->driver;
    pci->bus.alloc_resource(dev, type, rid, start, end, size, flags);
  } else if (type == RT_IOPORTS) {
    from = &isa->io_rman;
  } else if (type == RT_MEMORY) {
    panic("ISA resource cannot be `RT_MEMORY`");
  } else {
    panic("Unknown ISA resource type: %d", type);
  }

  resource_t *r = rman_alloc_resource(from, start, end, size, size, flags);
  if (r == NULL)
    return NULL;
  
  if ((flags & RF_ACTIVE) && (type == RT_IOPORTS)) {
    r->r_bus_tag = generic_bus_space;
    r->r_bus_handle = isa->io->r_bus_handle + r->r_start;
    rman_activate_resource(r);
  }

  return r;
}

static int intel_isa_activate_resource(device_t *dev, res_type_t type, int rid,
                                       resource_t *r) {
  assert(type != RT_MEMORY);
  /* Is there really anything to do here? */
}

static int intel_isa_release_resource(device_t *dev, res_type_t type, int rid,
                                       resource_t *r) {
  assert(type != RT_MEMORY);
  rman_release_resource(r);
}

static int intel_isa_probe(device_t *d) {
  return d->unit == 0;
}

static void intel_isa_intr_setup(device_t *dev, resource_t *r,
                                 ih_filter_t *filter, ih_service_t *service,
                                 void *arg, const char *name)  {
  assert(dev->parent->driver == &isa_bus->driver)
  intel_isa_state_t *isa = dev->parent->state;
  int irq = r->r_start;
  intr_event_add_handler(isa->intr_event[irq], filter, service, arg, name);
}

static void intel_isa_intr_teardown(device_t *isab, intr_handler_t *handler) {
  assert(isab->parent->driver == &intel_isa_bus.driver);
  intr_event_remove_handler(handler);
}

static int intel_isa_attach(device_t *isab) {
  intel_isa_state_t *isa = isab->state;
  isa->io =
    bus_alloc_resource(isab, RT_MEMORY, 0, 0, IO_ISASIZE,
                       0x1000, RF_ACTIVE);
  rman_init(&isa->io_rman, "Intel 82371AB ISA bridge I/O ports",
            0x0000, 0x0fff, RT_IOPORTS);
  isab->bus = DEV_BUS_ISA;
}

/* clang-format off */
bus_driver_t intel_isa_bus = {
  .driver = {
    .desc = "Intel 82371AB PIIX4 PCI to ISA bridge driver",
    .size = sizeof(intel_isa_state_t),
    .attach = intel_isa_attach,
    .probe = intel_isa_probe,
  },
  .bus = {
    .intr_setup = intel_isa_intr_setup,
    .intr_teardown = intel_isa_intr_teardown,
    .alloc_resource = intel_isa_alloc_resource,
    .activate_resource = intel_isa_activate_resource,
    .release_resource = intel_isa_release_resource,
  }
};
