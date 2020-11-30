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
  intr_event_t *intr_event[MIPS_NIRQ];
} rootdev_t;

static void rootdev_mask_irq(intr_event_t *ie) {
  int irq = ie->ie_irq;
  mips32_bc_c0(C0_STATUS, SR_IM0 << irq);
}

static void rootdev_unmask_irq(intr_event_t *ie) {
  int irq = ie->ie_irq;
  mips32_bs_c0(C0_STATUS, SR_IM0 << irq);
}

/* clang-format off */
static const char *rootdev_intr_name[MIPS_NIRQ] = {
  [MIPS_SWINT0] = "swint(0)",
  [MIPS_SWINT1] = "swint(1)",
  [MIPS_HWINT0] = "hwint(0)",
  [MIPS_HWINT1] = "hwint(1)",
  [MIPS_HWINT2] = "hwint(2)",
  [MIPS_HWINT3] = "hwint(3)",
  [MIPS_HWINT4] = "hwint(4)",
  [MIPS_HWINT5] = "hwint(5)",
};
/* clang-format on */

static void rootdev_intr_setup(device_t *dev, resource_t *r,
                               ih_filter_t *filter, ih_service_t *service,
                               void *arg, const char *name) {
  rootdev_t *rd = dev->parent->state;
  int irq = r->r_start;
  assert(irq < MIPS_NIRQ);

  if (rd->intr_event[irq] == NULL)
    rd->intr_event[irq] = intr_event_create(
      dev, irq, rootdev_mask_irq, rootdev_unmask_irq, rootdev_intr_name[irq]);

  r->r_handler =
    intr_event_add_handler(rd->intr_event[irq], filter, service, arg, name);
}

static void rootdev_intr_teardown(device_t *dev, resource_t *irq) {
  intr_event_remove_handler(irq->r_handler);

  /* TODO: should we remove empty interrupt event here and in every other
   * intr_teardown method? probably not... maybe in detach method? */
}

static resource_t *rootdev_alloc_resource(device_t *dev, res_type_t type,
                                          int rid, rman_addr_t start,
                                          rman_addr_t end, size_t size,
                                          res_flags_t flags) {
  rootdev_t *rd = dev->parent->state;
  rman_t *rman = NULL;
  size_t alignment = 0;

  if (type == RT_MEMORY) {
    rman = &rd->mem;
    alignment = PAGESIZE;
  } else if (type == RT_IRQ) {
    rman = &rd->irq;
  } else {
    panic("Resource type not handled!");
  }

  resource_t *r =
    rman_reserve_resource(rman, start, end, size, alignment, flags);
  if (r == NULL)
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
  if (type == RT_MEMORY)
    return bus_space_map(r->r_bus_tag, r->r_bus_handle, resource_size(r),
                         &r->r_bus_handle);

  return 0;
}

static void rootdev_intr_handler(ctx_t *ctx, device_t *dev, void *arg) {
  rootdev_t *rd = dev->state;
  unsigned pending = (_REG(ctx, CAUSE) & _REG(ctx, SR)) & CR_IP_MASK;

  for (int i = MIPS_NIRQ - 1; i >= 0; i--) {
    unsigned irq = CR_IP0 << i;

    if (pending & irq) {
      intr_event_run_handlers(rd->intr_event[i]);
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
  rman_init(&rd->mem, "Malta I/O space");
  rman_manage_region(&rd->mem, MALTA_PERIPHERALS_BASE, MALTA_PERIPHERALS_SIZE);

  rman_init(&rd->irq, "MIPS interrupts");
  rman_manage_region(&rd->irq, 0, MIPS_NIRQ);

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
