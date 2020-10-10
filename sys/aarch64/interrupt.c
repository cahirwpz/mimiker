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

/* TODO(pj): temporary hack -- remove it before merge */
extern void intr_tick(void);

void cpu_intr_handler(ctx_t *ctx) {
  intr_disable();
  /* TODO(pj): temporary hack -- remove it before merge */
  intr_tick();
  intr_enable();
  on_exc_leave();
}
