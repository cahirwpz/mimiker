#define KL_LOG KL_DEV
#include <sys/bus.h>
#include <sys/devclass.h>
#include <sys/errno.h>
#include <sys/fdt.h>
#include <sys/interrupt.h>
#include <sys/klog.h>
#include <dev/simplebus.h>
#include <riscv/mcontext.h>
#include <riscv/riscvreg.h>

typedef enum hlic_irq {
  HLIC_IRQ_SOFTWARE_USER,
  HLIC_IRQ_SOFTWARE_SUPERVISOR,
  HLIC_IRQ_SOFTWARE_HYPERVISOR,
  HLIC_IRQ_SOFTWARE_MACHINE,
  HLIC_IRQ_TIMER_USER,
  HLIC_IRQ_TIMER_SUPERVISOR,
  HLIC_IRQ_TIMER_HYPERVISOR,
  HLIC_IRQ_TIMER_MACHINE,
  HLIC_IRQ_EXTERNAL_USER,
  HLIC_IRQ_EXTERNAL_SUPERVISOR,
  HLIC_IRQ_EXTERNAL_HYPERVISOR,
  HLIC_IRQ_EXTERNAL_MACHINE,
  HLIC_NIRQS
} hlic_irq_t;

typedef struct rootdev {
  rman_t mem_rm;                        /* memory resource manager */
  rman_t hlic_rm;                       /* HLIC resource manager */
  intr_event_t *intr_event[HLIC_NIRQS]; /* HLIC interrupt events */
} rootdev_t;

static const char *hlic_intr_name[HLIC_NIRQS] = {
  [HLIC_IRQ_SOFTWARE_USER] = "user software",
  [HLIC_IRQ_SOFTWARE_SUPERVISOR] = "supervisor software",
  [HLIC_IRQ_SOFTWARE_HYPERVISOR] = "hypervisor software",
  [HLIC_IRQ_SOFTWARE_MACHINE] = "machine software",
  [HLIC_IRQ_TIMER_USER] = "user timer",
  [HLIC_IRQ_TIMER_SUPERVISOR] = "supervisor timer",
  [HLIC_IRQ_TIMER_HYPERVISOR] = "hypervisor timer",
  [HLIC_IRQ_TIMER_MACHINE] = "machine timer",
  [HLIC_IRQ_EXTERNAL_USER] = "user external",
  [HLIC_IRQ_EXTERNAL_SUPERVISOR] = "supervisor external",
  [HLIC_IRQ_EXTERNAL_HYPERVISOR] = "hypervisor external",
  [HLIC_IRQ_EXTERNAL_MACHINE] = "machine external",
};

/*
 * Hart-Level Interrupt Controller.
 */

static void hlic_intr_disable(intr_event_t *ie) {
  unsigned irq = ie->ie_irq;
  csr_clear(sie, 1 << irq);
}

static void hlic_intr_enable(intr_event_t *ie) {
  unsigned irq = ie->ie_irq;
  csr_set(sie, 1 << irq);
}

static void hlic_setup_intr(device_t *pic, device_t *dev, resource_t *r,
                            ih_filter_t *filter, ih_service_t *service,
                            void *arg, const char *name) {
  rootdev_t *rd = pic->state;
  unsigned irq = resource_start(r);
  assert(irq < HLIC_NIRQS);

  if (!rd->intr_event[irq])
    rd->intr_event[irq] = intr_event_create(
      NULL, irq, hlic_intr_disable, hlic_intr_enable, hlic_intr_name[irq]);

  r->r_handler =
    intr_event_add_handler(rd->intr_event[irq], filter, service, arg, name);
}

static void hlic_teardown_intr(device_t *pic, device_t *dev, resource_t *r) {
  intr_event_remove_handler(r->r_handler);
}

static resource_t *hlic_alloc_intr(device_t *pic, device_t *dev, int rid,
                                   unsigned irq, rman_flags_t flags) {
  rootdev_t *rd = pic->state;
  rman_t *rman = &rd->hlic_rm;

  return rman_reserve_resource(rman, RT_IRQ, rid, irq, irq, 1, 0, flags);
}

static void hlic_release_intr(device_t *pic, device_t *dev, resource_t *r) {
  resource_release(r);
}

static void hlic_intr_handler(ctx_t *ctx, device_t *bus) {
  rootdev_t *rd = bus->state;
  u_long cause = _REG(ctx, CAUSE) & SCAUSE_CODE;
  assert(cause < HLIC_NIRQS);

  intr_event_t *ie = rd->intr_event[cause];
  if (!ie)
    panic("Unknown HLIC interrupt %lx!", cause);

  intr_event_run_handlers(ie);

  if (cause != HLIC_IRQ_TIMER_SUPERVISOR &&
      cause != HLIC_IRQ_EXTERNAL_SUPERVISOR) {
    csr_clear(sip, ~(1 << cause));
  }
}

static int hlic_map_intr(device_t *pic, device_t *dev, phandle_t *intr,
                         int icells) {
  if (icells != 1)
    return -1;

  unsigned irq = *intr;

  /* Software interrupts. */
  if (irq == HLIC_IRQ_EXTERNAL_SUPERVISOR || irq == HLIC_IRQ_TIMER_SUPERVISOR ||
      irq == HLIC_IRQ_SOFTWARE_SUPERVISOR)
    return irq;

  /* Machine interrupts. */
  if (irq == HLIC_IRQ_TIMER_MACHINE)
    return HLIC_IRQ_TIMER_SUPERVISOR;
  if (irq == HLIC_IRQ_SOFTWARE_MACHINE)
    return HLIC_IRQ_SOFTWARE_SUPERVISOR;

  return -1;
}

/*
 * Root bus.
 */

static int rootdev_activate_resource(device_t *dev, resource_t *r) {
  assert(r->r_type == RT_MEMORY);
  return bus_space_map(r->r_bus_tag, resource_start(r),
                       roundup(resource_size(r), PAGESIZE), &r->r_bus_handle);
}

static void rootdev_deactivate_resource(device_t *dev, resource_t *r) {
  /* TODO: unmap mapped resources. */
}

static resource_t *rootdev_alloc_resource(device_t *dev, res_type_t type,
                                          int rid, rman_addr_t start,
                                          rman_addr_t end, size_t size,
                                          rman_flags_t flags) {
  rootdev_t *rd = dev->parent->state;
  rman_t *rman = &rd->mem_rm;
  size_t alignment = PAGESIZE;

  assert(type == RT_MEMORY);

  resource_t *r =
    rman_reserve_resource(rman, type, rid, start, end, size, alignment, flags);
  if (!r)
    return NULL;

  r->r_bus_tag = generic_bus_space;
  r->r_bus_handle = resource_start(r);

  if (flags & RF_ACTIVE) {
    if (bus_activate_resource(dev, r)) {
      resource_release(r);
      return NULL;
    }
  }

  return r;
}

static void rootdev_release_resource(device_t *dev, resource_t *r) {
  bus_deactivate_resource(dev, r);
  resource_release(r);
}

static int rootdev_probe(device_t *bus) {
  return 1;
}

static int rootdev_attach(device_t *bus) {
  rootdev_t *rd = bus->state;
  bus->node = 0;

  rman_init(&rd->mem_rm, "RISC-V I/O space");
  rman_manage_region(&rd->mem_rm, 0xf0000000, 0x10000000);

  /*
   * NOTE: supervisor can only control supervisor and user interrupts, however,
   * we don't support user-level trap extension.
   */
  rman_init(&rd->hlic_rm, "HLIC interrupts");
  rman_manage_region(&rd->hlic_rm, HLIC_IRQ_SOFTWARE_SUPERVISOR, 1);
  rman_manage_region(&rd->hlic_rm, HLIC_IRQ_TIMER_SUPERVISOR, 1);
  rman_manage_region(&rd->hlic_rm, HLIC_IRQ_EXTERNAL_SUPERVISOR, 1);

  intr_root_claim(hlic_intr_handler, bus);

  /*
   * Device enumeration.
   * TODO: this should be performed by a simplebus enumeration.
   */

  phandle_t node;
  int unit = 0;
  int err;

  /* PLIC */
  device_t *plic;
  if ((node = FDT_finddevice("/soc/interrupt-controller")) == FDT_NODEV)
    return ENXIO;
  if (!(err = simplebus_add_child(bus, node, unit++, bus, &plic)))
    return err;

  /* CLINT */
  if ((node = FDT_finddevice("/soc/clint")) == FDT_NODEV)
    return ENXIO;
  if (!(err = simplebus_add_child(bus, node, unit++, bus, NULL)))
    return err;

  /* UART */
  if ((node = FDT_finddevice("/soc/serial")) == FDT_NODEV)
    return ENXIO;
  if (!(err = simplebus_add_child(bus, node, unit++, plic, NULL)))
    return err;

  return bus_generic_probe(bus);
}

static pic_methods_t hlic_pic_if = {
  .alloc_intr = hlic_alloc_intr,
  .release_intr = hlic_release_intr,
  .setup_intr = hlic_setup_intr,
  .teardown_intr = hlic_teardown_intr,
  .map_intr = hlic_map_intr,
};

static bus_methods_t rootdev_bus_if = {
  .alloc_resource = rootdev_alloc_resource,
  .release_resource = rootdev_release_resource,
  .activate_resource = rootdev_activate_resource,
  .deactivate_resource = rootdev_deactivate_resource,
};

driver_t rootdev_driver = {
  .desc = "RISC-V root bus driver",
  .size = sizeof(rootdev_t),
  .pass = FIRST_PASS,
  .probe = rootdev_probe,
  .attach = rootdev_attach,
  .interfaces =
    {
      [DIF_PIC] = &hlic_pic_if,
      [DIF_BUS] = &rootdev_bus_if,
    },
};

DEVCLASS_CREATE(root);

/* XXX: We need at least one device in each devclass for the kernel to link
 * successfully. FTTB, let's just put `rootdev_driver` in `root` dc. */
DEVCLASS_ENTRY(root, rootdev_driver);
