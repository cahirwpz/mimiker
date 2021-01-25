/* 82371AB PCI-ISA bridge driver */

#define KL_LOG KL_DEV
#include <sys/mimiker.h>
#include <sys/device.h>
#include <sys/rman.h>
#include <sys/bus.h>
#include <mips/malta.h>
#include <sys/pci.h>
#include <sys/interrupt.h>
#include <dev/piixreg.h>
#include <dev/isareg.h>
#include <sys/libkern.h>
#include <sys/devclass.h>

typedef struct intel_isa_state {
  resource_t *io;
  rman_t io_rman;
} intel_isa_state_t;

static resource_t *intel_isa_alloc_resource(device_t *dev, res_type_t type,
                                            int rid, rman_addr_t start,
                                            rman_addr_t end, size_t size,
                                            rman_flags_t flags) {
  assert(dev->bus == DEV_BUS_ISA);

  rman_t *rman = NULL;
  intel_isa_state_t *isa = dev->parent->state;
  bus_space_handle_t bh = 0;

  if (type == RT_IOPORTS) {
    assert(start >= IO_ICUSIZE);
    rman = &isa->io_rman;
    bh = isa->io->r_bus_handle;
  } else {
    /* We don't know how to allocate this resource, but maybe someone above us
     * does. In current context this is necessary to allocate interrupts,
     * which aren't managed by this driver. */
    return bus_alloc_resource(dev->parent, type, rid, start, end, size, flags);
  }

  resource_t *r =
    rman_reserve_resource(rman, type, rid, start, end, size, 0, flags);

  if (r == NULL)
    return NULL;
  r->r_rid = rid;

  /* We shouldn't do that for IRQs, but rn we'll never end up here when
   * allocating a resouce */
  r->r_bus_tag = generic_bus_space;
  r->r_bus_handle = bh + resource_start(r);

  if (flags & RF_ACTIVE) {
    if (bus_activate_resource(dev, r)) {
      resource_release(r);
      return NULL;
    }
  }

  return r;
}

static int intel_isa_activate_resource(device_t *dev, resource_t *r) {
  if (r->r_type != RT_IOPORTS)
    return bus_activate_resource(dev->parent, r);
  /* There are no resource activation procedures for ISA ioports  */
  return 0;
}

static void intel_isa_deactivate_resource(device_t *dev, resource_t *r) {
  if (r->r_type != RT_IOPORTS)
    return bus_deactivate_resource(dev->parent, r);
  return;
}

static void intel_isa_release_resource(device_t *dev, resource_t *r) {
  if (r->r_type != RT_IOPORTS) {
    /* Dispatch release for inderectly allocated resources */
    bus_deactivate_resource(dev->parent, r);
  } else {
    /* No need to deactivate resource, because deactivation is a NOP anyway */
    resource_release(r);
  }
}

static int intel_isa_probe(device_t *d) {
  return d->unit == 0;
}

static int intel_isa_attach(device_t *isab) {
  intel_isa_state_t *isa = isab->state;
  isa->io = device_take_ioports(isab, 0, RF_ACTIVE);
  rman_init_from_resource(&isa->io_rman, "ISA IO ports", isa->io);

  /* -------------------------------------------------------------
   * Create child devices of ISA bus.
   */
  device_t *dev;

  /* atkbdc keyboard device */
  dev = device_add_child(isab, 0);
  dev->bus = DEV_BUS_ISA;
  device_add_ioports(dev, 0, IO_KBD, IO_KBDSIZE);
  device_add_irq(dev, 0, 1);

  /* ns16550 uart device */
  dev = device_add_child(isab, 1);
  dev->bus = DEV_BUS_ISA;
  device_add_ioports(dev, 0, IO_COM1, IO_COMSIZE);
  device_add_irq(dev, 0, 4);

  /* rtc device */
  dev = device_add_child(isab, 2);
  dev->bus = DEV_BUS_ISA;
  device_add_ioports(dev, 0, IO_RTC, IO_RTCSIZE);
  device_add_irq(dev, 0, 8);

  /* i8254 timer device */
  dev = device_add_child(isab, 3);
  dev->bus = DEV_BUS_ISA;
  device_add_ioports(dev, 0, IO_TIMER1, IO_TMRSIZE);
  device_add_irq(dev, 0, 0);

  return bus_generic_probe(isab);
}

bus_methods_t intel_isa_bus_bus_if = {
  .intr_setup = NULL,    /* Dispatched */
  .intr_teardown = NULL, /* Dispatched */
  .alloc_resource = intel_isa_alloc_resource,
  .activate_resource = intel_isa_activate_resource,
  .deactivate_resource = intel_isa_deactivate_resource,
  .release_resource = intel_isa_release_resource,
};

driver_t intel_isa_bus = {
  .desc = "Intel 82371AB PIIX4 PCI to ISA bridge driver",
  .size = sizeof(intel_isa_state_t),
  .attach = intel_isa_attach,
  .probe = intel_isa_probe,
  .pass = FIRST_PASS,
  .interfaces =
    {
      [DIF_BUS] = &intel_isa_bus_bus_if,
    },
};

DEVCLASS_ENTRY(pci, intel_isa_bus);