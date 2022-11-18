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
#include <riscv/vm_param.h>

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

static hlic_irq_t hlic_intr_map[HLIC_NIRQS] = {
  [HLIC_IRQ_SOFTWARE_USER] = -1,
  [HLIC_IRQ_SOFTWARE_SUPERVISOR] = HLIC_IRQ_SOFTWARE_SUPERVISOR,
  [HLIC_IRQ_SOFTWARE_HYPERVISOR] = -1,
  [HLIC_IRQ_SOFTWARE_MACHINE] = HLIC_IRQ_SOFTWARE_SUPERVISOR,
  [HLIC_IRQ_TIMER_USER] = -1,
  [HLIC_IRQ_TIMER_SUPERVISOR] = HLIC_IRQ_TIMER_SUPERVISOR,
  [HLIC_IRQ_TIMER_HYPERVISOR] = -1,
  [HLIC_IRQ_TIMER_MACHINE] = HLIC_IRQ_TIMER_SUPERVISOR,
  [HLIC_IRQ_EXTERNAL_SUPERVISOR] = HLIC_IRQ_EXTERNAL_SUPERVISOR,
  [HLIC_IRQ_EXTERNAL_USER] = -1,
  [HLIC_IRQ_EXTERNAL_HYPERVISOR] = -1,
  [HLIC_IRQ_EXTERNAL_MACHINE] = -1,
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
  unsigned irq = r->r_irq;
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

static void hlic_intr_handler(ctx_t *ctx, device_t *bus) {
  rootdev_t *rd = bus->state;
  u_long cause = _REG(ctx, CAUSE) & SCAUSE_CODE;
  assert(cause < HLIC_NIRQS);

  intr_event_t *ie = rd->intr_event[cause];
  if (!ie)
    panic("Unknown HLIC interrupt %lx!", cause);

  intr_event_run_handlers(ie);

  /* Supervisor software interrupts are cleared directly through SIP. */
  if (cause == HLIC_IRQ_SOFTWARE_SUPERVISOR)
    csr_clear(sip, ~(1 << cause));
}

static int hlic_map_intr(device_t *pic, device_t *dev, phandle_t *intr,
                         int icells) {
  if (icells != 1)
    return -1;

  unsigned irq = *intr;
  if (irq >= HLIC_NIRQS)
    return -1;
  return hlic_intr_map[irq];
}

/*
 * Root bus.
 */

static int rootdev_map_resource(device_t *dev, resource_t *r) {
  assert(r->r_type == RT_MEMORY);

  r->r_bus_tag = generic_bus_space;

  return bus_space_map(r->r_bus_tag, r->r_start,
                       roundup(resource_size(r), PAGESIZE), &r->r_bus_handle);
}

static void rootdev_unmap_resource(device_t *dev, resource_t *r) {
  assert(r->r_type == RT_MEMORY);
  /* TODO: unmap mapped resources. */
}

static int rootdev_probe(device_t *bus) {
  return 1;
}

static int rootdev_attach(device_t *bus) {
  bus->node = FDT_finddevice("/cpus/cpu/interrupt-controller");
  if (bus->node == FDT_NODEV)
    return ENXIO;

  intr_root_claim(hlic_intr_handler, bus);

  /*
   * Device enumeration.
   * TODO: this should be performed by a simplebus enumeration.
   */

  int unit = 0;
  int err;
  device_t *plic;

  if ((err = simplebus_add_child(bus, "/soc/interrupt-controller", unit++, bus,
                                 &plic)))
    return err;

  if ((err = simplebus_add_child(bus, "/soc/clint", unit++, bus, NULL)))
    return err;

  if ((err = simplebus_add_child(bus, "/soc/serial", unit++, plic, NULL)))
    return err;

  return bus_generic_probe(bus);
}

static pic_methods_t hlic_pic_if = {
  .setup_intr = hlic_setup_intr,
  .teardown_intr = hlic_teardown_intr,
  .map_intr = hlic_map_intr,
};

static bus_methods_t rootdev_bus_if = {
  .map_resource = rootdev_map_resource,
  .unmap_resource = rootdev_unmap_resource,
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
