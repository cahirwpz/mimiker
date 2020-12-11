/* 82371AB PCI-ISA bridge driver */

#define KL_LOG KL_DEV
#include <sys/isa.h>
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

#define IO_ISASIZE 1024

#define ICU_ADDR(x) ((x) + 0)
#define ICU_DATA(x) ((x) + 1)
#define ICU1_ADDR ICU_ADDR(IO_ICU1)
#define ICU1_DATA ICU_DATA(IO_ICU1)
#define ICU2_ADDR ICU_ADDR(IO_ICU2)
#define ICU2_DATA ICU_DATA(IO_ICU2)

#define ISA_IRQ_BASE (0)

bus_driver_t intel_isa_bus;


/* DEVCLASS_DECLARE(isa); */

typedef struct intel_isa_state {
  resource_t *io;
  rman_t io_rman;
  intr_event_t *intr_event[ICU_LEN];
} intel_isa_state_t;

static resource_t *intel_isa_alloc_resource(device_t *dev, res_type_t type,
                                            int rid, rman_addr_t start,
                                            rman_addr_t end, size_t size,
                                            res_flags_t flags) {
  rman_t *rman = NULL;
  intel_isa_state_t *isa = dev->parent->state;

  if (type == RT_IRQ) {
    assert(dev->bus == DEV_BUS_ISA);
    assert(start < IO_ICUSIZE);
    rman = &isa->io_rman;
  } else if (type == RT_IOPORTS) {
    assert(start >= IO_ICUSIZE);
    rman = &isa->io_rman;
  } else if (type == RT_MEMORY) {
    panic("ISA resource cannot be `RT_MEMORY`");
  } else {
    panic("Unknown ISA resource type: %d", type);
  }

  resource_t *r = rman_reserve_resource(rman, start, end, size, size, flags);
  if (r == NULL)
    return NULL;
  
  if (flags & RF_ACTIVE) {
    if (bus_activate_resource(dev, type, r)) {
      rman_release_resource(r);
      return NULL;
    }
  }

  return r;
}

static int intel_isa_activate_resource(device_t *dev, res_type_t type,
                                       resource_t *r) {
  assert(type != RT_MEMORY);
  /* ISA resources don't require any activation procedures. */
  return 0;
}

static void intel_isa_release_resource(device_t *dev, res_type_t type,
                                      resource_t *r) {
  assert(type != RT_MEMORY);
  rman_release_resource(r);
}

static int intel_isa_probe(device_t *d) {
  return d->unit == 4;
}

static void intel_isa_mask_irq(intr_event_t *ie) {
/*   intel_isa_state_t *sia = ie->ie_source;
  unsigned irq = ie->ie_irq;

  isa->imask |= (1 << irq);
  isa->elcr |= (1 << irq);
  gt_pci_set_icus(gtpci); */

  /* UNIIMPLEMENDTED!!!!!! */
}

static void intel_isa_unmask_irq(intr_event_t *ie) {
/*   gt_pci_state_t *gtpci = ie->ie_source;
  unsigned irq = ie->ie_irq;

  gtpci->imask &= ~(1 << irq);
  gtpci->elcr &= ~(1 << irq);
  gt_pci_set_icus(gtpci); */

  /* UNIIMPLEMENDTED!!!!!! */
}

static const char *intel_isa_intr_name[ICU_LEN] = {
  [0] = "timer",
  [1] = "kbd",        /* kbd controller (keyboard) */
  [2] = "pic-slave",  /* PIC cascade */
  [3] = "uart(1)",    /* COM 2 */
  [4] = "uart(0)",    /* COM 1 */
  [5] = "unused(0)",
  [6] = "floppy",     /* floppy */
  [7] = "parallel",   /* centronics */
  [8] = "rtc",        /* RTC */
  [9] = "i2c",        /* I2C */
  [10] = "unused(1)",
  [11] = "unused(2)",
  [12] = "mouse",     /* kbd controller (mouse) */
  [13] = "unused(3)",
  [14] = "ide(0)",    /* IDE primary */
  [15] = "ide(1)",    /* IDE secondary */
};

static void intel_isa_intr_setup(device_t *dev, resource_t *r,
                                 ih_filter_t *filter, ih_service_t *service,
                                 void *arg, const char *name)  {
  assert(dev->parent->driver == &intel_isa_bus.driver);
  intel_isa_state_t *isa = dev->parent->state;
  int irq = r->r_start;
  assert(irq < ISA_IRQ_BASE + ICU_LEN && irq >= ISA_IRQ_BASE);
  
  if (isa->intr_event[irq] == NULL)
      isa->intr_event[irq] = intr_event_create(
        isa, irq, intel_isa_mask_irq, intel_isa_unmask_irq,
        intel_isa_intr_name[irq]);

  r->r_handler =
    intr_event_add_handler(isa->intr_event[irq], filter, service, arg, name);
}

static void intel_isa_intr_teardown(device_t *isab, resource_t *irq) {
  assert(isab->parent->driver == &intel_isa_bus.driver);
  intr_event_remove_handler(irq->r_handler);
}

static int intel_isa_attach(device_t *isab) {
  intel_isa_state_t *isa = isab->state;
  isa->io = device_take_ioports(isab, 0, RF_ACTIVE);
  rman_init_from_resource(&isa->io_rman, "ISA IO ports", isa->io);
  rman_manage_region(&isa->io_rman, 0, IO_ISAEND);
  /* isab->devclass = &DEVCLASS(isa); */

  /* -------------------------------------------------------------
   * Create child devices of ISA bus.
   */
  device_t *dev;

  /* atkbdc keyboard device */
  dev = device_add_child(isab, 0);
  dev->bus = DEV_BUS_ISA; /* XXX: ISA device workaround */
  device_add_ioports(dev, 0, IO_KBD, IO_KBDSIZE);
  device_add_irq(dev, 0, ISA_IRQ_BASE + 1);

  /* ns16550 uart device */
  dev = device_add_child(isab, 1);
  dev->bus = DEV_BUS_ISA; /* XXX: ISA device workaround */
  device_add_ioports(dev, 0, IO_COM1, IO_COMSIZE);
  device_add_irq(dev, 0, ISA_IRQ_BASE + 4);

  /* rtc device */
  dev = device_add_child(isab, 2);
  dev->bus = DEV_BUS_ISA; /* XXX: ISA device workaround */
  device_add_ioports(dev, 0, IO_RTC, IO_RTCSIZE);
  device_add_irq(dev, 0, ISA_IRQ_BASE + 8);

  /* i8254 timer device */
  dev = device_add_child(isab, 3);
  dev->bus = DEV_BUS_ISA; /* XXX: ISA device workaround */
  device_add_ioports(dev, 0, IO_TIMER1, IO_TMRSIZE);
  device_add_irq(dev, 0, ISA_IRQ_BASE + 0);

  return bus_generic_probe(isab);
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

DEVCLASS_ENTRY(pci, intel_isa_bus);