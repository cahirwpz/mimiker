#define KL_LOG KL_DEV
#include <sys/bus.h>
#include <sys/devclass.h>
#include <sys/dtb.h>
#include <sys/fdt.h>
#include <sys/interrupt.h>
#include <sys/klog.h>
#include <riscv/mcontext.h>
#include <riscv/riscvreg.h>

typedef enum {
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
  rman_t mem_rm;                        /* memory resource manager*/
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

static void hlic_intr_setup(device_t *ic, device_t *dev, resource_t *r,
                            ih_filter_t *filter, ih_service_t *service,
                            void *arg, const char *name) {
  rootdev_t *rd = ic->state;
  unsigned irq = resource_start(r);
  assert(irq < HLIC_NIRQS);

  if (!rd->intr_event[irq])
    rd->intr_event[irq] = intr_event_create(
      NULL, irq, hlic_intr_disable, hlic_intr_enable, hlic_intr_name[irq]);

  r->r_handler =
    intr_event_add_handler(rd->intr_event[irq], filter, service, arg, name);
}

static void hlic_intr_teardown(device_t *ic, device_t *dev, resource_t *r) {
  intr_event_remove_handler(r->r_handler);
}

static resource_t *hlic_intr_alloc(device_t *ic, device_t *dev, int rid,
                                   unsigned irq) {
  rootdev_t *rd = ic->state;
  rman_t *rman = &rd->hlic_rm;

  return rman_reserve_resource(rman, RT_IRQ, rid, irq, irq, 1, 0, 0);
}

static void hlic_intr_release(device_t *ic, device_t *dev, resource_t *r) {
  resource_release(r);
}

static void hlic_intr_handler(ctx_t *ctx, device_t *bus, void *arg) {
  rootdev_t *rd = bus->state;
  unsigned long cause = _REG(ctx, CAUSE) & SCAUSE_CODE;
  assert(cause < HLIC_NIRQS);

  intr_event_t *ie = rd->intr_event[cause];
  if (!ie)
    panic("Unknown HLIC interrupt %u!", cause);

  intr_event_run_handlers(ie);
  csr_clear(sip, 1 << cause);
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
  void *dtb = dtb_root();
  int soc_node = dtb_offset("soc");
  int node, len;
  const uint32_t *prop;

  /* Obtain I/O space boundaries. */
  paddr_t io_start = 0;
  paddr_t io_end = 0;
  int first = fdt_first_subnode(dtb, soc_node);
  dtb_reg(first, &prop, NULL);
  io_start = fdt32_to_cpu(*prop);
  int last = -1;
  for (node = first; node >= 0; node = fdt_next_subnode(dtb, node)) {
    last = node;
  }
  dtb_reg(last, &prop, &len);
  io_end = fdt32_to_cpu(prop[0]) + fdt32_to_cpu(prop[1]);

  rman_init(&rd->mem_rm, "RISC-V I/O space");
  rman_manage_region(&rd->mem_rm, io_start, io_end - io_start);

  /*
   * NOTE: supervisor can only control supervisor and user interrupts, however,
   * we don't support user-level trap extension.
   */
  rman_init(&rd->hlic_rm, "HLIC interrupts");
  rman_manage_region(&rd->hlic_rm, HLIC_IRQ_SOFTWARE_SUPERVISOR, 1);
  rman_manage_region(&rd->hlic_rm, HLIC_IRQ_TIMER_SUPERVISOR, 1);
  rman_manage_region(&rd->hlic_rm, HLIC_IRQ_EXTERNAL_SUPERVISOR, 1);

  intr_root_claim(hlic_intr_handler, bus, NULL);

  /* TODO(MichalBlk): discover devices using FDT. */

  /* Create RISC-V CLINT device and assign resources to it. */
  device_t *dev = device_add_child(bus, 1);
  dev->ic = bus;
  node = fdt_subnode_offset(dtb, soc_node, "clint");
  assert(node >= 0);
  dtb_reg(node, &prop, &len);
  device_add_memory(dev, 0, fdt32_to_cpu(prop[0]), fdt32_to_cpu(prop[1]));
  device_add_irq(dev, 0, HLIC_IRQ_SOFTWARE_SUPERVISOR);
  device_add_irq(dev, 1, HLIC_IRQ_TIMER_SUPERVISOR);
  dev->node = node;

  /* Create RISC-V PLIC device and assign resources to it. */
  device_t *plic = device_add_child(bus, 2);
  plic->ic = bus;
  node = fdt_subnode_offset(dtb, soc_node, "interrupt-controller");
  assert(node >= 0);
  dtb_reg(node, &prop, &len);
  device_add_memory(plic, 0, fdt32_to_cpu(prop[0]), fdt32_to_cpu(prop[1]));
  device_add_irq(plic, 0, HLIC_IRQ_EXTERNAL_SUPERVISOR);
  plic->node = node;

  extern driver_t plic_driver;
  plic->driver = &plic_driver;
  assert(device_probe(plic));
  assert(!device_attach(plic));

  /* Create liteuart device and assign resources to it. */
  dev = device_add_child(bus, 0);
  dev->ic = plic;
  node = fdt_subnode_offset(dtb, soc_node, "serial");
  assert(node >= 0);
  dtb_reg(node, &prop, &len);
  device_add_memory(dev, 0, fdt32_to_cpu(prop[0]), fdt32_to_cpu(prop[1]));
  dtb_intr(node, &prop, &len);
  device_add_irq(dev, 0, fdt32_to_cpu(prop[0]));
  dev->node = node;

  return bus_generic_probe(bus);
}

static ic_methods_t hlic_ic_if = {
  .intr_alloc = hlic_intr_alloc,
  .intr_release = hlic_intr_release,
  .intr_setup = hlic_intr_setup,
  .intr_teardown = hlic_intr_teardown,
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
      [DIF_IC] = &hlic_ic_if,
      [DIF_BUS] = &rootdev_bus_if,
    },
};

DEVCLASS_CREATE(root);
