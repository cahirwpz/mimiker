#define KL_LOG KL_DEV
#include <sys/bus.h>
#include <sys/devclass.h>
#include <sys/errno.h>
#include <sys/fdt.h>
#include <sys/interrupt.h>
#include <sys/klog.h>
#include <sys/libkern.h>

/*
 * Details of the operation of PLIC as well as the memory layout
 * can be found in the official specification which is maintained on github:
 *   https://github.com/riscv/riscv-plic-spec
 */

/*
 * For the VexRiscv hardware platform we assume a single HART
 * with two PLIC contexts:
 *  - ctx0 - machine mode
 *  - ctx1 - supervisor mode
 *
 * The organization of PLIC contexts for SiFive Unleashed can be found
 * in the official manual (chapter 10):
 *  -
 * https://sifive.cdn.prismic.io/sifive/d3ed5cd0-6e74-46b2-a12d-72b06706513e_fu540-c000-manual-v1p4.pdf
 *
 * NOTE: FTTB, we designate a single HART to handle interrupts from all
 * peripheral-level devices.
 */

#if __riscv_xlen == 64
#define PLIC_CTXNUM_SV 2
#else
#define PLIC_CTXNUM_SV 1
#endif

/* PLIC memory map. */

#define PLIC_PRIORITY_BASE 0x000000

#define PLIC_ENABLE_BASE 0x002000
#define PLIC_ENABLE_STRIDE 0x80
#define PLIC_ENABLE_BASE_SV                                                    \
  (PLIC_ENABLE_BASE + PLIC_CTXNUM_SV * PLIC_ENABLE_STRIDE)

#define PLIC_CONTEXT_BASE 0x200000
#define PLIC_CONTEXT_STRIDE 0x1000
#define PLIC_CONTEXT_THRESHOLD 0x0
#define PLIC_CONTEXT_CLAIM 0x4
#define PLIC_CONTEXT_BASE_SV                                                   \
  (PLIC_CONTEXT_BASE + PLIC_CTXNUM_SV * PLIC_CONTEXT_STRIDE)

#define PLIC_PRIORITY(n) (PLIC_PRIORITY_BASE + (n) * sizeof(uint32_t))
#define PLIC_ENABLE_SV(n) (PLIC_ENABLE_BASE_SV + ((n) / 32) * sizeof(uint32_t))
#define PLIC_THRESHOLD_SV (PLIC_CONTEXT_BASE_SV + PLIC_CONTEXT_THRESHOLD)
#define PLIC_CLAIM_SV (PLIC_CONTEXT_BASE_SV + PLIC_CONTEXT_CLAIM)

typedef struct plic_state {
  resource_t *mem;           /* PLIC memory resource */
  resource_t *irq;           /* PLIC irq resource */
  intr_event_t **intr_event; /* interrupt events */
  unsigned ndev;             /* number of sources */
} plic_state_t;

#define in4(addr) bus_read_4(plic->mem, (addr))
#define out4(addr, val) bus_write_4(plic->mem, (addr), (val))

static void plic_intr_disable(intr_event_t *ie) {
  plic_state_t *plic = ie->ie_source;
  unsigned irq = ie->ie_irq;

  uint32_t en = in4(PLIC_ENABLE_SV(irq));
  en &= ~(1 << (irq % 32));
  out4(PLIC_ENABLE_SV(irq), en);
}

static void plic_intr_enable(intr_event_t *ie) {
  plic_state_t *plic = ie->ie_source;
  unsigned irq = ie->ie_irq;

  uint32_t en = in4(PLIC_ENABLE_SV(irq));
  en |= 1 << (irq % 32);
  out4(PLIC_ENABLE_SV(irq), en);
}

static void plic_setup_intr(device_t *pic, device_t *dev, resource_t *r,
                            ih_filter_t *filter, ih_service_t *service,
                            void *arg, const char *name) {
  plic_state_t *plic = pic->state;
  unsigned irq = r->r_irq;
  assert(irq && irq < plic->ndev);

  char buf[32];
  snprintf(buf, sizeof(buf), "PLIC source %u", irq);

  if (!plic->intr_event[irq])
    plic->intr_event[irq] =
      intr_event_create(plic, irq, plic_intr_disable, plic_intr_enable, buf);

  r->r_handler =
    intr_event_add_handler(plic->intr_event[irq], filter, service, arg, name);
}

static void plic_teardown_intr(device_t *pic, device_t *dev, resource_t *r) {
  intr_event_remove_handler(r->r_handler);
}

static int plic_map_intr(device_t *pic, device_t *dev, phandle_t *intr,
                         int icells) {
  plic_state_t *plic = pic->state;

  if (icells != 1)
    return -1;

  unsigned irq = *intr;
  if (!irq || irq > plic->ndev)
    return -1;

  return irq;
}

static intr_filter_t plic_intr_handler(void *arg) {
  plic_state_t *plic = arg;

  /* Claim any pending interrupt. */
  uint32_t irq = in4(PLIC_CLAIM_SV);

  if (!irq)
    panic("Asserted reserved PLIC interrupt!");

  intr_event_run_handlers(plic->intr_event[irq]);
  /* Complete the interrupt. */
  out4(PLIC_CLAIM_SV, irq);

  return IF_FILTERED;
}

static int plic_probe(device_t *pic) {
  return FDT_is_compatible(pic->node, "riscv,plic0") ||
         FDT_is_compatible(pic->node, "sifive,plic-1.0.0") ||
         FDT_is_compatible(pic->node, "sifive,fu540-c000-plic");
}

static int plic_attach(device_t *pic) {
  plic_state_t *plic = pic->state;
  int err = 0;

  /* Obtain the number of sources. */
  if (FDT_getencprop(pic->node, "riscv,ndev", (void *)&plic->ndev,
                     sizeof(uint32_t)) != sizeof(uint32_t))
    return ENXIO;

  /* We'll need interrupt event for each interrupt source. */
  plic->intr_event =
    kmalloc(M_DEV, plic->ndev * sizeof(intr_event_t *), M_WAITOK | M_ZERO);
  if (!plic->intr_event)
    return ENXIO;

  plic->mem = device_take_memory(pic, 0);
  assert(plic->mem);

  if ((err = bus_map_resource(pic, plic->mem)))
    return err;

  /*
   * In case PLIC supports priorities, set each priority to 1
   * and the threshold to 0.
   */
  for (unsigned irq = 0; irq < plic->ndev; irq++) {
    out4(PLIC_PRIORITY(irq), 1);
  }
  out4(PLIC_THRESHOLD_SV, 0);

  plic->irq = device_take_irq(pic, 0);
  assert(plic->irq);

  pic_setup_intr(pic, plic->irq, plic_intr_handler, NULL, plic, "PLIC");

  return 0;
}

static pic_methods_t plic_pic_if = {
  .setup_intr = plic_setup_intr,
  .teardown_intr = plic_teardown_intr,
  .map_intr = plic_map_intr,
};

driver_t plic_driver = {
  .desc = "RISC-V PLIC driver",
  .size = sizeof(plic_state_t),
  .pass = FIRST_PASS,
  .probe = plic_probe,
  .attach = plic_attach,
  .interfaces =
    {
      [DIF_PIC] = &plic_pic_if,
    },
};

DEVCLASS_ENTRY(root, plic_driver);
