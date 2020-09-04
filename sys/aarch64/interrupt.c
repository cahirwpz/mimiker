#include <sys/mimiker.h>
#include <aarch64/interrupt.h>
#include <aarch64/armreg.h>

void cpu_intr_disable(void) {
  __asm __volatile("msr daifset, #2");
}

void cpu_intr_enable(void) {
  __asm __volatile("msr daifclr, #2");
}

bool cpu_intr_disabled(void) {
  uint32_t daif = READ_SPECIALREG(daif);
  return (daif & 2) == 0;
}
