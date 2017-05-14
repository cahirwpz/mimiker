#include <mips/malta.h>
#include <interrupt.h>
#include <bus.h>
#include <sync.h>
#include <exception.h>

extern device_t *rootdev;

typedef struct rootdev { intr_chain_t chain[8]; } rootdev_t;

static inline rootdev_t *rootdev_of(device_t *dev) {
  return dev->instance;
}

extern const char _ebase[];

static void mips_intr_init() {
  /*
   * Enable Vectored Interrupt Mode as described in „MIPS32® 24KETM Processor
   * Core Family Software User’s Manual”, chapter 6.3.1.2.
   */

  /* The location of exception vectors is set to EBase. */
  mips32_set_c0(C0_EBASE, _ebase);
  mips32_bc_c0(C0_STATUS, SR_BEV);
  /* Use the special interrupt vector at EBase + 0x200. */
  mips32_bs_c0(C0_CAUSE, CR_IV);
  /* Set vector spacing to 0. */
  mips32_set_c0(C0_INTCTL, INTCTL_VS_0);
}

static void mips_intr_setup(device_t *dev, unsigned num,
                            intr_handler_t *handler) {
  intr_chain_t *chain = &rootdev_of(dev)->chain[num];
  critical_enter();
  intr_chain_add_handler(chain, handler);
  if (chain->ic_count == 1)
    mips32_bs_c0(C0_STATUS, SR_IM0 << num);
  critical_leave();
}

static void mips_intr_teardown(device_t *dev, intr_handler_t *handler) {
  intr_chain_t *chain = handler->ih_chain;
  critical_enter();
  if (chain->ic_count == 1)
    mips32_bc_c0(C0_STATUS, SR_IM0 << chain->ic_irq);
  intr_chain_remove_handler(handler);
  critical_leave();
}

void mips_irq_handler(exc_frame_t *frame) {
  unsigned pending = (frame->cause & frame->sr) & CR_IP_MASK;
  intr_chain_t *chain = rootdev_of(rootdev)->chain;

  for (int i = 7; i >= 0; i--) {
    unsigned irq = CR_IP0 << i;

    if (pending & irq) {
      intr_chain_run_handlers(&chain[i]);
      pending &= ~irq;
    }
  }

  mips32_set_c0(C0_CAUSE, frame->cause & ~CR_IP_MASK);
}

void rootdev_init() {
  TAILQ_INIT(&rootdev->children);

  mips_intr_init();

  {
    intr_chain_t *chain = rootdev_of(rootdev)->chain;

    /* Initialize software interrupts handler chains. */
    intr_chain_init(&chain[0], 0, "swint(0)");
    intr_chain_init(&chain[1], 1, "swint(1)");

    /* Initialize hardware interrupts handler chains. */
    intr_chain_init(&chain[2], 2, "hwint(0)");
    intr_chain_init(&chain[3], 3, "hwint(1)");
    intr_chain_init(&chain[4], 4, "hwint(2)");
    intr_chain_init(&chain[5], 5, "hwint(3)");
    intr_chain_init(&chain[6], 6, "hwint(4)");
    intr_chain_init(&chain[7], 7, "hwint(5)");
  }
}

bus_driver_t rootdev_driver = {
  .driver =
    {
      .size = sizeof(rootdev_t), .desc = "MIPS platform root bus driver",
    },
  .bus =
    {
      .intr_setup = mips_intr_setup, .intr_teardown = mips_intr_teardown,
    }};

/* globally visible root device */
device_t *rootdev = &(device_t){
  .driver = (driver_t *)&rootdev_driver, .instance = &(rootdev_t){},
};
