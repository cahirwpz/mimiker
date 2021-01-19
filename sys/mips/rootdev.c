#define KL_LOG KL_DEV
#include <sys/klog.h>
#include <mips/malta.h>
#include <mips/mips.h>
#include <mips/m32c0.h>
#include <mips/interrupt.h>
#include <sys/bus.h>
#include <sys/exception.h>
#include <sys/interrupt.h>
#include <sys/devclass.h>
#include <mips/mcontext.h>

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
  int irq = resource_start(r);
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
  size_t alignment = 0;
  rman_t *rman = NULL;

  if (type == RT_MEMORY) {
    alignment = PAGESIZE;
    rman = &rd->mem;
  } else if (type == RT_IRQ) {
    rman = &rd->irq;
  } else {
    panic("Resource type not handled!");
  }

  resource_t *r = kmalloc(M_DEV, sizeof(resource_t), M_WAITOK);
  r->r_type = type;
  r->r_rid = rid;
  r->r_res = rman_reserve_resource(rman, start, end, size, alignment, flags);
  if (r->r_res == NULL)
    goto bad;

  if (type == RT_MEMORY) {
    r->r_bus_tag = generic_bus_space;
    r->r_bus_handle = resource_start(r);
  }

  if (flags & RF_ACTIVE) {
    if (bus_activate_resource(dev, r)) {
      rman_release_resource(r->r_res);
      goto bad;
    }
  }

  return r;

bad:
  kfree(M_DEV, r);
  return NULL;
}

static void rootdev_release_resource(device_t *dev, resource_t *r) {
  bus_deactivate_resource(dev, r);
  rman_release_resource(r->r_res);
  kfree(M_DEV, r);
}

static int rootdev_activate_resource(device_t *dev, resource_t *r) {
  if (r->r_type == RT_MEMORY)
    return bus_space_map(r->r_bus_tag, resource_start(r), resource_size(r),
                         &r->r_bus_handle);

  return 0;
}

static void rootdev_deactivate_resource(device_t *dev, resource_t *r) {
  /* TODO: unmap mapped resources. */
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

static int rootdev_attach(device_t *bus) {
  rootdev_t *rd = bus->state;

  /* Manages space occupied by I/O devices: PCI, FPGA, system controler, ...
   * Skips region allocated for up to 256MB of RAM. */
  rman_init(&rd->mem, "Malta I/O space");
  rman_manage_region(&rd->mem, MALTA_PERIPHERALS_BASE, MALTA_PERIPHERALS_SIZE);

  rman_init(&rd->irq, "MIPS interrupts");
  rman_manage_region(&rd->irq, 0, MIPS_NIRQ);

  intr_root_claim(rootdev_intr_handler, bus, NULL);

  /* Create MIPS timer device and assign resources to it. */
  device_t *dev = device_add_child(bus, 0);
  device_add_irq(dev, 0, MIPS_HWINT5);

  /* Create GT PCI device and assign resources to it. */
  dev = device_add_child(bus, 1);
  dev->bus = DEV_BUS_PCI;
  dev->devclass = &DEVCLASS(pci);
  /* PCI I/O memory. */
  device_add_memory(dev, 0, MALTA_PCI0_MEMORY_BASE, MALTA_PCI0_MEMORY_SIZE);
  /* PCI I/O ports 0x0000-0xffff. */
  device_add_memory(dev, 1, MALTA_PCI0_IO_BASE, 0x10000);
  /* GT64120 registers. */
  device_add_memory(dev, 2, MALTA_CORECTRL_BASE, MALTA_CORECTRL_SIZE);
  /* GT64120 main irq. */
  device_add_irq(dev, 0, MIPS_HWINT0);

  /* TODO: replace raw resource assignments by parsing FDT file. */

  return bus_generic_probe(bus);
}

static bus_methods_t rootdev_bus_if = {
  .intr_setup = rootdev_intr_setup,
  .intr_teardown = rootdev_intr_teardown,
  .alloc_resource = rootdev_alloc_resource,
  .release_resource = rootdev_release_resource,
  .activate_resource = rootdev_activate_resource,
  .deactivate_resource = rootdev_deactivate_resource,
};

driver_t rootdev_driver = {
  .desc = "MIPS platform root bus driver",
  .size = sizeof(rootdev_t),
  .pass = FIRST_PASS,
  .probe = rootdev_probe,
  .attach = rootdev_attach,
  .interfaces =
    {
      [DIF_BUS] = &rootdev_bus_if,
    },
};

DEVCLASS_CREATE(root);
