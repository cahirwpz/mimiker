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

typedef struct rootdev_device {
  resource_list_t resources;
} rootdev_device_t;

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

  if (type == RT_MEMORY)
    rman = &rd->mem;
  else if (type == RT_IRQ)
    rman = &rd->irq;
  else
    panic("Resource type not handled!");

  resource_t *r = resource_list_alloc(dev, rman, type, rid, flags);
  if (r == NULL)
    return NULL;

  if (type == RT_MEMORY) {
    r->r_bus_tag = generic_bus_space;
    r->r_bus_handle = r->r_start;
  }

  if (flags & RF_ACTIVE) {
    if (bus_activate_resource(dev, type, rid, r)) {
      resource_list_release(dev, type, rid, r);
      return NULL;
    }
  }

  return r;
}

static void rootdev_release_resource(device_t *dev, res_type_t type, int rid,
                                     resource_t *r) {
  /* TODO: we should unmap mapped resources. */
  resource_list_release(dev, type, rid, r);
}

static int rootdev_activate_resource(device_t *dev, res_type_t type, int rid,
                                     resource_t *r) {
  if (type == RT_MEMORY)
    return bus_space_map(r->r_bus_tag, r->r_bus_handle, rman_get_size(r),
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

DEVCLASS_DECLARE(pci);

static device_t *rootdev_add_child(device_t *bus, device_bus_t db,
                                   devclass_t *dc, int unit) {
  device_t *dev = device_add_child(bus, NULL, unit);
  rootdev_device_t *rdd = kmalloc(M_DEV, sizeof(rootdev_device_t), M_WAITOK);
  assert(dev && rdd);

  dev->bus = db;
  dev->devclass = dc;
  dev->instance = rdd;
  resource_list_init(dev);

  return dev;
}

static int rootdev_attach(device_t *bus) {
  rootdev_t *rd = bus->state;

  /* Manages space occupied by I/O devices: PCI, FPGA, system controler, ...
   * Skips region allocated for up to 256MB of RAM. */
  rman_init(&rd->mem, "Malta I/O space", MALTA_PCI0_MEMORY_BASE, MALTA_FPGA_END,
            RT_MEMORY);
  rman_init(&rd->irq, "MIPS interrupts", 0, MIPS_NIRQ - 1, RT_IRQ);

  intr_root_claim(rootdev_intr_handler, bus, NULL);

  /* Create MIPS timer device and assign resources to it. */
  device_t *dev = rootdev_add_child(bus, DEV_BUS_NONE, NULL, 0);
  resource_list_add_irq(dev, 0, MIPS_HWINT5);

  /* Create GT PCI device and assign resources to it. */
  dev = rootdev_add_child(bus, DEV_BUS_PCI, &DEVCLASS(pci), 1);
  /* PCI I/O memory. */
  resource_list_add(dev, RT_MEMORY, 0, MALTA_PCI0_MEMORY_BASE,
                    MALTA_PCI0_MEMORY_SIZE);
  /* PCI I/O ports 0x0000-0xffff. */
  resource_list_add(dev, RT_MEMORY, 1, MALTA_PCI0_IO_BASE, 0x10000);
  /* GT64120 registers. */
  resource_list_add(dev, RT_MEMORY, 2, MALTA_CORECTRL_BASE,
                    MALTA_CORECTRL_SIZE);
  /* GT64120 main irq. */
  resource_list_add_irq(dev, 0, MIPS_HWINT0);

  /* TODO: replace raw resource assignments by parsing FDT file. */

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
