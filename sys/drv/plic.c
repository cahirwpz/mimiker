#define KL_LOG KL_DEV
#include <sys/devclass.h>
#include <sys/dtb.h>
#include <sys/errno.h>
#include <sys/fdt.h>
#include <sys/interrupt.h>
#include <sys/klog.h>

/* PLIC memory map. */
#define PLIC_PRIORITY_BASE 0x000000
#define PLIC_ENABLE_BASE 0x002000
#define PLIC_CONTEXT_BASE 0x200000
#define PLIC_CONTEXT_THRESHOLD 0x0
#define PLIC_CONTEXT_CLAIM 0x4

#define PLIC_PRIORITY(n) (PLIC_PRIORITY_BASE + (n) * sizeof(uint32_t))
#define PLIC_ENABLE(n) (PLIC_ENABLE_BASE + ((n) / 32) * sizeof(uint32_t))
#define PLIC_THRESHOLD (PLIC_CONTEXT_BASE + PLIC_CONTEXT_THRESHOLD)
#define PLIC_CLAIM (PLIC_CONTEXT_BASE + PLIC_CONTEXT_CLAIM)

typedef struct plic_state {
  rman_t rm;                 /* irq resource manager */
  resource_t *mem;           /* PLIC memory resource */
  resource_t *irq;           /* PLIC irq resource */
  intr_event_t **intr_event; /* interrupt events */
  unsigned nirqs;            /* number of sources */
} plic_state_t;

#define in32(addr) bus_read_4(plic->mem, (addr))
#define out32(addr, val) bus_write_4(plic->mem, (addr), (val))

static void plic_intr_disable(intr_event_t *ie) {
  plic_state_t *plic = ie->ie_source;
  unsigned irq = ie->ie_irq;

  uint32_t en = in32(PLIC_ENABLE(irq));
  en &= ~(1 << (irq % 32));
  out32(PLIC_ENABLE(irq), en);
}

static void plic_intr_enable(intr_event_t *ie) {
  plic_state_t *plic = ie->ie_source;
  unsigned irq = ie->ie_irq;

  uint32_t en = in32(PLIC_ENABLE(irq));
  en |= 1 << (irq % 32);
  out32(PLIC_ENABLE(irq), en);
}

static const char *plic_intr_name(unsigned irq) {
  char buf[32];
  snprintf(buf, sizeof(buf), "PLIC source %u", irq);
  return kstrndup(M_STR, buf, sizeof(buf));
}

static void plic_intr_setup(device_t *ic, device_t *dev, resource_t *r,
                            ih_filter_t *filter, ih_service_t *service,
                            void *arg, const char *name) {
  plic_state_t *plic = ic->state;
  unsigned irq = resource_start(r);
  assert(irq && irq < plic->nirqs);

  if (!plic->intr_event[irq])
    plic->intr_event[irq] = intr_event_create(
      plic, irq, plic_intr_disable, plic_intr_enable, plic_intr_name(irq));

  r->r_handler =
    intr_event_add_handler(plic->intr_event[irq], filter, service, arg, name);
}

static void plic_intr_teardown(device_t *ic, device_t *dev, resource_t *r) {
  intr_event_remove_handler(r->r_handler);
}

static resource_t *plic_intr_alloc(device_t *ic, device_t *dev, int rid,
                                   unsigned irq) {
  plic_state_t *plic = ic->state;
  rman_t *rman = &plic->rm;

  return rman_reserve_resource(rman, RT_IRQ, rid, irq, irq, 1, 0, 0);
}

static void plic_intr_release(device_t *ic, device_t *dev, resource_t *r) {
  resource_release(r);
}

static intr_filter_t plic_intr_handler(void *arg) {
  plic_state_t *plic = arg;

  /* Claim any pending interrupt. */
  uint32_t irq = in32(PLIC_CLAIM);
  if (irq) {
    intr_event_run_handlers(plic->intr_event[irq]);
    /* Complete the interrupt. */
    out32(PLIC_CLAIM, irq);
    return IF_FILTERED;
  }

  return IF_STRAY;
}

static int plic_probe(device_t *ic) {
  void *dtb = dtb_root();
  const char *compatible = fdt_getprop(dtb, ic->node, "compatible", NULL);
  if (!compatible)
    return 0;
  return strcmp(compatible, "riscv,plic0") == 0;
}

static int plic_attach(device_t *ic) {
  plic_state_t *plic = ic->state;

  /* Obtain the number of sources. */
  int len;
  void *dtb = dtb_root();
  const uint32_t *prop = fdt_getprop(dtb, ic->node, "riscv,ndev", &len);
  if (!prop || len != sizeof(uint32_t))
    return ENXIO;
  plic->nirqs = fdt32_to_cpu(*prop);

  /* We'll need interrupt event for each interrupt source. */
  plic->intr_event =
    kmalloc(M_DEV, plic->nirqs * sizeof(intr_event_t *), M_WAITOK | M_ZERO);
  if (!plic->intr_event)
    return ENXIO;

  rman_init(&plic->rm, "PLIC interrupt sources");
  rman_manage_region(&plic->rm, 1, plic->nirqs);

  plic->mem = device_take_memory(ic, 0, RF_ACTIVE);
  assert(plic->mem);

  /*
   * In case the PLIC supports priorities, set each priority to 1
   * and the threshold to 0.
   */
  for (unsigned irq = 0; irq < plic->nirqs; irq++) {
    out32(PLIC_PRIORITY(irq), 1);
  }
  out32(PLIC_THRESHOLD, 0);

  plic->irq = device_take_irq(ic, 0, RF_ACTIVE);
  assert(plic->irq);

  intr_setup(ic, plic->irq, plic_intr_handler, NULL, plic, "PLIC");

  return 0;
}

static ic_methods_t plic_ic_if = {
  .intr_alloc = plic_intr_alloc,
  .intr_release = plic_intr_release,
  .intr_setup = plic_intr_setup,
  .intr_teardown = plic_intr_teardown,
};

driver_t plic_driver = {
  .desc = "RISC-V PLIC driver",
  .size = sizeof(plic_state_t),
  .pass = FIRST_PASS,
  .probe = plic_probe,
  .attach = plic_attach,
  .interfaces =
    {
      [DIF_IC] = &plic_ic_if,
    },
};

DEVCLASS_ENTRY(root, plic_driver);
