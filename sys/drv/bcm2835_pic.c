#define KL_LOG KL_DEV
#include <sys/bus.h>
#include <sys/devclass.h>
#include <sys/errno.h>
#include <sys/interrupt.h>
#include <sys/klog.h>
#include <sys/libkern.h>
#include <dev/bcm2835reg.h>
#include <dev/fdt_dev.h>

/*
 * located at BCM2835_ARMICU_BASE
 * accessed by BCM2835_PERIPHERALS_BASE NOT by BCM2835_PERIPHERALS_BASE_BUS
 * 32 GPU0 interrupts -- 0x204 offset from address; 0 in table
 * 32 GPU1 interrupts -- 0x208 offset from address; 32 in table
 * 32 basic interrupts -- 0x200 offset from address; 64 in table
 */

/* NOTE: the values of the following enum must not be modified! */
typedef enum bcm2835_intrtype {
  BCM2835_INTRTYPE_BASIC,
  BCM2835_INTRTYPE_GPU0,
  BCM2835_INTRTYPE_GPU1,
  BCM2835_INTRTYPE_CNT
} bcm2835_intrtype_t;

#define BCM2835_NIRQPERTYPE (BCM2835_NIRQ / BCM2835_INTRTYPE_CNT)

typedef struct bcm2835_pic_state {
  rman_t rm;
  resource_t *mem;
  resource_t *irq;
  intr_event_t *intr_event[BCM2835_NIRQ];
} bcm2835_pic_state_t;

#define in4(addr) bus_read_4(bcm2835_pic->mem, (addr))
#define out4(addr, val) bus_write_4(bcm2835_pic->mem, (addr), (val))

static void bcm2835_pic_disable_irq(intr_event_t *ie) {
  bcm2835_pic_state_t *bcm2835_pic = ie->ie_source;
  unsigned irq = ie->ie_irq;

  bus_size_t reg = BCM2835_INTC_IRQBDISABLE;
  if (irq < BCM2835_INT_GPU1BASE)
    reg = BCM2835_INTC_IRQ1DISABLE;
  else if (irq < BCM2835_INT_BASICBASE)
    reg = BCM2835_INTC_IRQ2DISABLE;

  uint32_t en = in4(reg);
  out4(reg, en & ~(1 << irq));
}

static void bcm2835_pic_enable_irq(intr_event_t *ie) {
  bcm2835_pic_state_t *bcm2835_pic = ie->ie_source;
  unsigned irq = ie->ie_irq;

  bus_size_t reg = BCM2835_INTC_IRQBENABLE;
  if (irq < BCM2835_INT_GPU1BASE)
    reg = BCM2835_INTC_IRQ1ENABLE;
  else if (irq < BCM2835_INT_BASICBASE)
    reg = BCM2835_INTC_IRQ2ENABLE;

  uint32_t en = in4(reg);
  out4(reg, en | (1 << irq));
}

static resource_t *bcm2835_pic_alloc_intr(device_t *pic, device_t *dev, int rid,
                                          unsigned irq, rman_flags_t flags) {
  bcm2835_pic_state_t *bcm2835_pic = pic->state;
  rman_t *rman = &bcm2835_pic->rm;
  return rman_reserve_resource(rman, RT_IRQ, rid, irq, irq, 1, 0, flags);
}

static void bcm2835_pic_release_intr(device_t *pic, device_t *dev,
                                     resource_t *r) {
  resource_release(r);
}

static const char *bcm2835_pic_intr_name(unsigned irq) {
  const char *type = "BASIC";
  if (irq < BCM2835_INT_GPU1BASE)
    type = "GPU0";
  else if (irq < BCM2835_INT_BASICBASE)
    type = "GPU1";
  return kasprintf("%s source %u", type, irq);
}

static void bcm2835_pic_setup_intr(device_t *pic, device_t *dev, resource_t *r,
                                   ih_filter_t *filter, ih_service_t *service,
                                   void *arg, const char *name) {
  bcm2835_pic_state_t *bcm2835_pic = pic->state;
  unsigned irq = resource_start(r);
  assert(irq < BCM2835_NIRQ);

  if (bcm2835_pic->intr_event[irq] == NULL)
    bcm2835_pic->intr_event[irq] =
      intr_event_create(bcm2835_pic, irq, bcm2835_pic_disable_irq,
                        bcm2835_pic_enable_irq, bcm2835_pic_intr_name(irq));

  r->r_handler = intr_event_add_handler(bcm2835_pic->intr_event[irq], filter,
                                        service, arg, name);
}

static void bcm2835_pic_teardown_intr(device_t *pic, device_t *dev,
                                      resource_t *irq) {
  intr_event_remove_handler(irq->r_handler);
}

static int bcm2835_pic_map_intr(device_t *pic, device_t *dev, phandle_t *intr,
                                int icells) {
  if (icells != 2)
    return -1;

  /* Interrupt tuples are of the form `<intrtype irq_offset>`. */
  bcm2835_intrtype_t type = intr[0];
  unsigned irq_off = intr[1];

  if (type >= BCM2835_INTRTYPE_CNT)
    return -1;
  if (irq_off >= BCM2835_NIRQPERTYPE)
    return -1;

  /*
   * We need to translate the relative interrupt source number
   * to an absolute one. See the comment at the top of this file for
   * the absolute layout of the interrupt sources.
   */
  unsigned irq = irq_off;
  if (type == BCM2835_INTRTYPE_GPU1)
    irq += BCM2835_NIRQPERTYPE;
  else if (type == BCM2835_INTRTYPE_BASIC)
    irq += BCM2835_NIRQPERTYPE * 2;
  return irq;
}

static int bcm2835_pic_handle_intrtype(bcm2835_pic_state_t *bcm2835_pic,
                                       bus_size_t reg, intr_event_t **events) {
  uint32_t pending = in4(reg);
  int rv = (pending != 0);

  while (pending) {
    unsigned irq = ffs(pending) - 1;
    /* XXX: some pending bits are shared between BASIC and GPU0/1. */
    if (events[irq])
      intr_event_run_handlers(events[irq]);
    pending &= ~(1 << irq);
  }

  return rv;
}

static intr_filter_t bcm2835_pic_intr_handler(void *arg) {
  bcm2835_pic_state_t *bcm2835_pic = arg;

  int gpu0_intr = bcm2835_pic_handle_intrtype(
    bcm2835_pic, BCM2835_INTC_IRQ1PENDING, bcm2835_pic->intr_event);
  int gpu1_intr =
    bcm2835_pic_handle_intrtype(bcm2835_pic, BCM2835_INTC_IRQ2PENDING,
                                &bcm2835_pic->intr_event[BCM2835_INT_GPU1BASE]);
  int basic_intr = bcm2835_pic_handle_intrtype(
    bcm2835_pic, BCM2835_INTC_IRQBPENDING,
    &bcm2835_pic->intr_event[BCM2835_INT_BASICBASE]);

  return (gpu0_intr || gpu1_intr || basic_intr) ? IF_FILTERED : IF_STRAY;
}

static int bcm2835_pic_probe(device_t *pic) {
  return FDT_dev_is_compatible(pic, "brcm,bcm2836-armctrl-ic");
}

static int bcm2835_pic_attach(device_t *pic) {
  bcm2835_pic_state_t *bcm2835_pic = pic->state;

  rman_init(&bcm2835_pic->rm, "BCM2835 PIC interrupt sources");
  rman_manage_region(&bcm2835_pic->rm, 0, BCM2835_NIRQ);

  bcm2835_pic->mem = device_take_memory(pic, 0, RF_ACTIVE);
  assert(bcm2835_pic->mem);

  bcm2835_pic->irq = device_take_irq(pic, 0, RF_ACTIVE);
  assert(bcm2835_pic->irq);

  pic_setup_intr(pic, bcm2835_pic->irq, bcm2835_pic_intr_handler, NULL,
                 bcm2835_pic, "BCM2835 PIC");

  return 0;
}

static pic_methods_t bcm2835_pic_if = {
  .alloc_intr = bcm2835_pic_alloc_intr,
  .release_intr = bcm2835_pic_release_intr,
  .setup_intr = bcm2835_pic_setup_intr,
  .teardown_intr = bcm2835_pic_teardown_intr,
  .map_intr = bcm2835_pic_map_intr,
};

static driver_t bcm2835_pic_driver = {
  .desc = "BCM2835 PIC driver",
  .size = sizeof(bcm2835_pic_state_t),
  .pass = FIRST_PASS,
  .probe = bcm2835_pic_probe,
  .attach = bcm2835_pic_attach,
  .interfaces =
    {
      [DIF_PIC] = &bcm2835_pic_if,
    },
};

DEVCLASS_ENTRY(simplebus, bcm2835_pic_driver);
