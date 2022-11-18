/* 82371AB PCI-ISA bridge driver */

#define KL_LOG KL_DEV
#include <sys/klog.h>
#include <sys/mimiker.h>
#include <sys/device.h>
#include <sys/bus.h>
#include <dev/malta.h>
#include <dev/pci.h>
#include <dev/piixreg.h>
#include <dev/isareg.h>
#include <sys/libkern.h>
#include <sys/devclass.h>

typedef struct intel_isa_state {
  resource_t *io;
} intel_isa_state_t;

static int intel_isa_map_resource(device_t *dev, resource_t *r) {
  assert(r->r_type == RT_IOPORTS);

  intel_isa_state_t *isa = dev->parent->state;
  bus_space_handle_t bh = isa->io->r_bus_handle;

  r->r_bus_tag = generic_bus_space;
  r->r_bus_handle = bh + r->r_start;

  return 0;
}

static void intel_isa_unmap_resource(device_t *dev, resource_t *r) {
  assert(r->r_type == RT_IOPORTS);
  /* IOports on ISA don't require deactivation */
}

static int intel_isa_probe(device_t *d) {
  return d->unit == 0;
}

static int intel_isa_attach(device_t *isab) {
  intel_isa_state_t *isa = isab->state;
  isa->io = device_take_ioports(isab, 0);
  int err = 0;

  if ((err = bus_map_resource(isab, isa->io)))
    return err;

  /* -------------------------------------------------------------
   * Create child devices of ISA bus.
   */
  device_t *dev;

  /* atkbdc keyboard device */
  dev = device_add_child(isab, 0);
  dev->pic = isab->pic;
  dev->bus = DEV_BUS_ISA;
  device_add_ioports(dev, 0, IO_KBD, IO_KBD + IO_KBDSIZE);
  device_add_irq(dev, 0, 1);

  /* ns16550 uart device */
  dev = device_add_child(isab, 1);
  dev->pic = isab->pic;
  dev->bus = DEV_BUS_ISA;
  device_add_ioports(dev, 0, IO_COM1, IO_COM1 + IO_COMSIZE);
  device_add_irq(dev, 0, 4);

  /* rtc device */
  dev = device_add_child(isab, 2);
  dev->pic = isab->pic;
  dev->bus = DEV_BUS_ISA;
  device_add_ioports(dev, 0, IO_RTC, IO_RTC + IO_RTCSIZE);
  device_add_irq(dev, 0, 8);

  /* i8254 timer device */
  dev = device_add_child(isab, 3);
  dev->pic = isab->pic;
  dev->bus = DEV_BUS_ISA;
  device_add_ioports(dev, 0, IO_TIMER1, IO_TIMER1 + IO_TMRSIZE);
  device_add_irq(dev, 0, 0);

  return bus_generic_probe(isab);
}

static bus_methods_t intel_isa_bus_bus_if = {
  .map_resource = intel_isa_map_resource,
  .unmap_resource = intel_isa_unmap_resource,
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
