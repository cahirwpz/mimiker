#define KL_LOG KL_DEV
#include <sys/klog.h>
#include <mips/malta.h>
#include <mips/mips.h>
#include <mips/context.h>
#include <mips/interrupt.h>
#include <sys/bus.h>
#include <sys/exception.h>
#include <sys/interrupt.h>
#include <sys/devclass.h>

typedef struct rootdev {
  rman_t mem, irq;
  intr_event_t intr_event[MIPS_NIRQ];
} rootdev_t;

static void rootdev_mask_irq(intr_event_t *ie) {
  int irq = ie->ie_irq;
  mips32_bc_c0(C0_STATUS, SR_IM0 << irq);
}

static void rootdev_unmask_irq(intr_event_t *ie) {
  int irq = ie->ie_irq;
  mips32_bs_c0(C0_STATUS, SR_IM0 << irq);
}

static void rootdev_intr_setup(device_t *dev, unsigned irq,
                               intr_handler_t *handler) {
  rootdev_t *rd = dev->parent->state;
  intr_event_t *event = &rd->intr_event[irq];
  intr_event_add_handler(event, handler);
}

static void rootdev_intr_teardown(device_t *dev, intr_handler_t *handler) {
  intr_event_remove_handler(handler);
}

static resource_t *rootdev_alloc_resource(device_t *dev, res_type_t type,
                                          int rid, rman_addr_t start,
                                          rman_addr_t end, size_t size,
                                          res_flags_t flags) {
  rootdev_t *rd = dev->parent->state;
  rman_t *rman = NULL;
  resource_t *r;

  if (type == RT_MEMORY)
    rman = &rd->mem;
  else if (type == RT_IRQ)
    rman = &rd->irq;
  else
    panic("Resource type not handled!");

  if (!(r = rman_alloc_resource(rman, start, end, size, 1, flags)))
    return NULL;

  if (type == RT_MEMORY) {
    r->r_bus_tag = generic_bus_space;
    r->r_bus_handle = r->r_start;
  }

  if (flags & RF_ACTIVE) {
    if (bus_activate_resource(dev, type, rid, r)) {
      rman_release_resource(r);
      return NULL;
    }
  }

  return r;
}

static void rootdev_release_resource(device_t *dev, res_type_t type, int rid,
                                     resource_t *r) {
  panic("not implemented!");
}

static int rootdev_activate_resource(device_t *dev, res_type_t type, int rid,
                                     resource_t *r) {
  int error = 0;

  if (r->r_flags & RF_ACTIVE)
    return 0;

  if (type == RT_MEMORY)
    error = bus_space_map(r->r_bus_tag, r->r_bus_handle, rman_get_size(r),
                          &r->r_bus_handle);

  if (error == 0)
    rman_activate_resource(r);

  return error;
}

static void rootdev_intr_handler(ctx_t *ctx, device_t *dev, void *arg) {
  rootdev_t *rd = dev->state;
  unsigned pending = (_REG(ctx, CAUSE) & _REG(ctx, SR)) & CR_IP_MASK;

  for (int i = MIPS_NIRQ - 1; i >= 0; i--) {
    unsigned irq = CR_IP0 << i;

    if (pending & irq) {
      intr_event_run_handlers(&rd->intr_event[i]);
      pending &= ~irq;
    }
  }
}

static int rootdev_probe(device_t *bus) {
  return 1;
}

static int rootdev_attach(device_t *bus) {
  rootdev_t *rd = bus->state;

  /* Manages space occupied by I/O devices: PCI, FPGA, system controler, ...
   * Skips region allocated for up to 256MB of RAM. */
  rman_init(&rd->mem, "Malta I/O space", MALTA_PCI0_MEMORY_BASE, MALTA_FPGA_END,
            RT_MEMORY);
  rman_init(&rd->irq, "MIPS interrupts", 0, MIPS_NIRQ - 1, RT_IRQ);

#define MIPS_INTR_EVENT(rd, irq, name)                                         \
  intr_event_init(&(rd)->intr_event[irq], irq, name, rootdev_mask_irq,         \
                  rootdev_unmask_irq, NULL)

  /* Initialize software interrupts handler events. */
  MIPS_INTR_EVENT(rd, MIPS_SWINT0, "swint(0)");
  MIPS_INTR_EVENT(rd, MIPS_SWINT1, "swint(1)");
  /* Initialize hardware interrupts handler events. */
  MIPS_INTR_EVENT(rd, MIPS_HWINT0, "hwint(0)");
  MIPS_INTR_EVENT(rd, MIPS_HWINT1, "hwint(1)");
  MIPS_INTR_EVENT(rd, MIPS_HWINT2, "hwint(2)");
  MIPS_INTR_EVENT(rd, MIPS_HWINT3, "hwint(3)");
  MIPS_INTR_EVENT(rd, MIPS_HWINT4, "hwint(4)");
  MIPS_INTR_EVENT(rd, MIPS_HWINT5, "hwint(5)");

#undef MIPS_INTR_EVENT

  for (unsigned i = 0; i < MIPS_NIRQ; i++)
    intr_event_register(&rd->intr_event[i]);

  intr_root_claim(rootdev_intr_handler, bus, NULL);

  (void)device_add_child(bus, NULL, 0); /* for MIPS timer */
  (void)device_add_child(bus, NULL, 1); /* for GT PCI */

  return bus_generic_probe(bus);
}

static bus_driver_t rootdev_driver = {
  .driver =
    {
      .size = sizeof(rootdev_t),
      .desc = "MIPS platform root bus driver",
      .probe = rootdev_probe,
      .attach = rootdev_attach,
    },
  .bus = {.intr_setup = rootdev_intr_setup,
          .intr_teardown = rootdev_intr_teardown,
          .alloc_resource = rootdev_alloc_resource,
          .release_resource = rootdev_release_resource,
          .activate_resource = rootdev_activate_resource}};

DEVCLASS_CREATE(root);

void init_devices(void) {
  static device_t rootdev;

  device_init(&rootdev, &DEVCLASS(root), 0);
  rootdev.driver = (driver_t *)&rootdev_driver;
  (void)device_probe(&rootdev);
  device_attach(&rootdev);
}
