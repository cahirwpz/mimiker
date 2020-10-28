#include <sys/mimiker.h>
#include <sys/context.h>
#include <sys/interrupt.h>
#include <sys/exception.h>
#include <aarch64/interrupt.h>
#include <aarch64/armreg.h>

#define _DAIF(x) ((DAIF_##x##_MASKED) >> 6)

void cpu_intr_disable(void) {
  __asm __volatile("msr daifset, %0" ::"i"(_DAIF(I)));
}

void cpu_intr_enable(void) {
  __asm __volatile("msr daifclr, %0" ::"i"(_DAIF(I)));
}

bool cpu_intr_disabled(void) {
  uint32_t daif = READ_SPECIALREG(daif);
  return (daif & DAIF_I_MASKED) != 0;
}

/* TODO(pj) check if it is a correct value */
#define N_IRQ 64

static intr_event_t aarch64_intr_event[N_IRQ];

void init_aarch64_intr(void) {
  for (int i = 0; i < N_IRQ; i++) {
    intr_event_init(&aarch64_intr_event[i], i, NULL, NULL, NULL, NULL);
    intr_event_register(&aarch64_intr_event[i]);
  }
}

void aarch64_intr_setup(intr_handler_t *handler, unsigned irq) {
  intr_event_t *event = &aarch64_intr_event[irq];
  intr_event_add_handler(event, handler);
}

void aarch64_intr_teardown(intr_handler_t *handler) {
  intr_event_remove_handler(handler);
}

void cpu_intr_handler(ctx_t *ctx) {
  intr_disable();

  /* TODO(pj) handle interrupts */
  panic("cpu_intr_handler");

  intr_enable();
  on_exc_leave();
}
