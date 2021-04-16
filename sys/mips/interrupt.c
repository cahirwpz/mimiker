#include <sys/interrupt.h>
#include <sys/sched.h>
#include <mips/mips.h>
#include <mips/m32c0.h>
#include <mips/interrupt.h>

/* Extra information regarding DI / EI usage (from MIPS® ISA documentation):
 *
 * The instruction creates an execution hazard between the change to SR register
 * and the point where the change to the interrupt enable takes effect. This
 * hazard is cleared by the EHB, JALR.HB, JR.HB, or ERET instructions. Software
 * must not assume that a fixed latency will clear the execution hazard. */

__no_instrument_kgprof void cpu_intr_disable(void) {
  asm volatile("di; ehb");
}

__no_instrument_kgprof void cpu_intr_enable(void) {
  asm volatile("ei; ehb");
}

__no_instrument_kgprof bool cpu_intr_disabled(void) {
  return (mips32_getsr() & SR_IE) == 0;
}
