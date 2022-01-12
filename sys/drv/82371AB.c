/* 82371AB PCI-ISA bridge driver */

#define KL_LOG KL_DEV
#include <sys/klog.h>
#include <sys/mimiker.h>
#include <sys/device.h>
#include <sys/rman.h>
#include <sys/bus.h>
#include <dev/malta.h>
#include <dev/pci.h>
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

  if (type != RT_IOPORTS) {
    assert(type == RT_IRQ);
    device_t *idev = BUS_METHOD_PROVIDER(dev->parent, alloc_resource);
    return bus_methods(idev->parent)
      ->alloc_resource(idev, type, rid, start, end, size, flags);
  }

  assert(start >= IO_ICUSIZE);

  intel_isa_state_t *isa = dev->parent->state;
  rman_t *rman = &isa->io_rman;
  bus_space_handle_t bh = isa->io->r_bus_handle;

  resource_t *r =
    rman_reserve_resource(rman, type, rid, start, end, size, 0, flags);

  if (r == NULL)
    return NULL;

  /* We shouldn't do that for IRQs, but rn we'll never end up here when
   * allocating them, because they get dispatched. */
  r->r_bus_tag = generic_bus_space;
  r->r_bus_handle = bh + resource_start(r);

  /* IOPorts are always active */
  if (bus_activate_resource(dev, r)) {
    resource_release(r);
    return NULL;
  }

  return r;
}

static int intel_isa_activate_resource(device_t *dev, resource_t *r) {
  assert(r->r_type == RT_IOPORTS || r->r_type == RT_IRQ);
  /* Neither IRQs in general or IOports on ISA require activation */
  return 0;
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

static bus_methods_t intel_isa_bus_bus_if = {
  .intr_setup = NULL,    /* Dispatched */
  .intr_teardown = NULL, /* Dispatched */
  .alloc_resource = intel_isa_alloc_resource,
  .activate_resource = intel_isa_activate_resource,
};

static driver_t intel_isa_bus = {
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
