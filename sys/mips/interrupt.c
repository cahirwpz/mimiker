#include <sys/interrupt.h>
#include <mips/context.h>
#include <mips/interrupt.h>

/* Extra information regarding DI / EI usage (from MIPS® ISA documentation):
 *
 * The instruction creates an execution hazard between the change to SR register
 * and the point where the change to the interrupt enable takes effect. This
 * hazard is cleared by the EHB, JALR.HB, JR.HB, or ERET instructions. Software
 * must not assume that a fixed latency will clear the execution hazard. */

void cpu_intr_disable(void) {
  asm volatile("di; ehb");
}

void cpu_intr_enable(void) {
  asm volatile("ei; ehb");
}

bool cpu_intr_disabled(void) {
  return (mips32_getsr() & SR_IE) == 0;
}

static intr_event_t mips_intr_event[8];

#define MIPS_INTR_EVENT(irq, name)                                             \
  intr_event_init(&mips_intr_event[irq], irq, name, mips_mask_irq,             \
                  mips_unmask_irq, NULL)

static void mips_mask_irq(intr_event_t *ie) {
  int irq = ie->ie_irq;
  mips32_bc_c0(C0_STATUS, SR_IM0 << irq);
}

static void mips_unmask_irq(intr_event_t *ie) {
  int irq = ie->ie_irq;
  mips32_bs_c0(C0_STATUS, SR_IM0 << irq);
}

void init_mips_intr(void) {
  /* Initialize software interrupts handler events. */
  MIPS_INTR_EVENT(MIPS_SWINT0, "swint(0)");
  MIPS_INTR_EVENT(MIPS_SWINT1, "swint(1)");
  /* Initialize hardware interrupts handler events. */
  MIPS_INTR_EVENT(MIPS_HWINT0, "hwint(0)");
  MIPS_INTR_EVENT(MIPS_HWINT1, "hwint(1)");
  MIPS_INTR_EVENT(MIPS_HWINT2, "hwint(2)");
  MIPS_INTR_EVENT(MIPS_HWINT3, "hwint(3)");
  MIPS_INTR_EVENT(MIPS_HWINT4, "hwint(4)");
  MIPS_INTR_EVENT(MIPS_HWINT5, "hwint(5)");

  for (unsigned i = 0; i < 8; i++)
    intr_event_register(&mips_intr_event[i]);
}

void mips_intr_setup(intr_handler_t *handler, unsigned irq) {
  intr_event_t *event = &mips_intr_event[irq];
  intr_event_add_handler(event, handler);
}

void mips_intr_teardown(intr_handler_t *handler) {
  intr_event_remove_handler(handler);
}

/* Hardware interrupt handler is called with interrupts disabled. */
void mips_intr_handler(ctx_t *ctx) {
  unsigned pending =
    (ctx->__gregs[_REG_CAUSE] & ctx->__gregs[_REG_SR]) & CR_IP_MASK;

  for (int i = 7; i >= 0; i--) {
    unsigned irq = CR_IP0 << i;

    if (pending & irq) {
      intr_event_run_handlers(&mips_intr_event[i]);
      pending &= ~irq;
    }
  }
}
