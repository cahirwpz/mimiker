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
  dev_mmio_t *io;
} intel_isa_state_t;

static int intel_isa_map_mmio(device_t *dev, dev_mmio_t *mmio) {
  intel_isa_state_t *isa = dev->parent->state;
  bus_space_handle_t bh = isa->io->bus_handle;

  mmio->bus_tag = generic_bus_space;
  mmio->bus_handle = bh + mmio->start;

  return 0;
}

static void intel_isa_unmap_mmio(device_t *dev, dev_mmio_t *mmio) {
  /* IOports on ISA don't require deactivation */
}

static int intel_isa_probe(device_t *d) {
  return d->unit == 0;
}

DEVCLASS_DECLARE(isa);

static int intel_isa_attach(device_t *isab) {
  intel_isa_state_t *isa = isab->state;
  unsigned pic_id = isab->parent->node;
  int err = 0;

  isab->devclass = &DEVCLASS(isa);

  if ((err = device_claim_mmio(isab, 0, &isa->io)))
    return err;

  /* -------------------------------------------------------------
   * Create child devices of ISA bus.
   */
  device_t *dev;

  /* atkbdc keyboard device */
  dev = device_add_child(isab, 0);
  dev->bus = DEV_BUS_ISA;
  device_add_mmio(dev, 0, IO_KBD, IO_KBD + IO_KBDSIZE, 0);
  device_add_intr(dev, 0, pic_id, 1);
  device_add_pending(dev);

  /* ns16550 uart device */
  dev = device_add_child(isab, 1);
  dev->bus = DEV_BUS_ISA;
  device_add_mmio(dev, 0, IO_COM1, IO_COM1 + IO_COMSIZE, 0);
  device_add_intr(dev, 0, pic_id, 4);
  device_add_pending(dev);

  /* rtc device */
  dev = device_add_child(isab, 2);
  dev->bus = DEV_BUS_ISA;
  device_add_mmio(dev, 0, IO_RTC, IO_RTC + IO_RTCSIZE, 0);
  device_add_intr(dev, 0, pic_id, 8);
  device_add_pending(dev);

  /* i8254 timer device */
  dev = device_add_child(isab, 3);
  dev->bus = DEV_BUS_ISA;
  device_add_mmio(dev, 0, IO_TIMER1, IO_TIMER1 + IO_TMRSIZE, 0);
  device_add_intr(dev, 0, pic_id, 0);
  device_add_pending(dev);

  return 0;
}

static bus_methods_t intel_isa_bus_bus_if = {
  .map_mmio = intel_isa_map_mmio,
  .unmap_mmio = intel_isa_unmap_mmio,
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
