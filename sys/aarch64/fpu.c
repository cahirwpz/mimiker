#include <aarch64/armreg.h>
#include <aarch64/context.h>
#include <aarch64/fpu.h>
#include <sys/mimiker.h>

bool fpu_enabled(void) {
  uint32_t cpacr = READ_SPECIALREG(cpacr_el1);
  return (cpacr & CPACR_FPEN_MASK) == CPACR_FPEN_TRAP_NONE;
}

void fpu_enable(void) {
  assert(fpu_enabled() == false);

  uint32_t cpacr;

  cpacr = READ_SPECIALREG(cpacr_el1);
  cpacr &= ~CPACR_FPEN_MASK;
  cpacr |= CPACR_FPEN_TRAP_NONE;
  WRITE_SPECIALREG(cpacr_el1, cpacr);
}

void fpu_disable(void) {
  assert(fpu_enabled());

  uint32_t cpacr;

  cpacr = READ_SPECIALREG(cpacr_el1);
  cpacr &= ~CPACR_FPEN_MASK;
  cpacr |= CPACR_FPEN_TRAP_ALL1;
  WRITE_SPECIALREG(cpacr_el1, cpacr);
}

void fpu_save_ctx(user_ctx_t *uctx) {
  assert(fpu_enabled());

  __asm __volatile("mrs    %0, fpcr        \n"
                   "mrs    %1, fpsr        \n"
                   "stp    q0,  q1,  [%2, #16 *  0]\n"
                   "stp    q2,  q3,  [%2, #16 *  2]\n"
                   "stp    q4,  q5,  [%2, #16 *  4]\n"
                   "stp    q6,  q7,  [%2, #16 *  6]\n"
                   "stp    q8,  q9,  [%2, #16 *  8]\n"
                   "stp    q10, q11, [%2, #16 * 10]\n"
                   "stp    q12, q13, [%2, #16 * 12]\n"
                   "stp    q14, q15, [%2, #16 * 14]\n"
                   "stp    q16, q17, [%2, #16 * 16]\n"
                   "stp    q18, q19, [%2, #16 * 18]\n"
                   "stp    q20, q21, [%2, #16 * 20]\n"
                   "stp    q22, q23, [%2, #16 * 22]\n"
                   "stp    q24, q25, [%2, #16 * 24]\n"
                   "stp    q26, q27, [%2, #16 * 26]\n"
                   "stp    q28, q29, [%2, #16 * 28]\n"
                   "stp    q30, q31, [%2, #16 * 30]\n"
                   : "=&r"(uctx->__fregs.__fpcr), "=&r"(uctx->__fregs.__fpsr)
                   : "r"(uctx->__fregs.__qregs));
}

void fpu_load_ctx(user_ctx_t *uctx) {
  assert(fpu_enabled());

  __asm __volatile("ldp    q0,  q1,  [%2, #16 *  0]\n"
                   "ldp    q2,  q3,  [%2, #16 *  2]\n"
                   "ldp    q4,  q5,  [%2, #16 *  4]\n"
                   "ldp    q6,  q7,  [%2, #16 *  6]\n"
                   "ldp    q8,  q9,  [%2, #16 *  8]\n"
                   "ldp    q10, q11, [%2, #16 * 10]\n"
                   "ldp    q12, q13, [%2, #16 * 12]\n"
                   "ldp    q14, q15, [%2, #16 * 14]\n"
                   "ldp    q16, q17, [%2, #16 * 16]\n"
                   "ldp    q18, q19, [%2, #16 * 18]\n"
                   "ldp    q20, q21, [%2, #16 * 20]\n"
                   "ldp    q22, q23, [%2, #16 * 22]\n"
                   "ldp    q24, q25, [%2, #16 * 24]\n"
                   "ldp    q26, q27, [%2, #16 * 26]\n"
                   "ldp    q28, q29, [%2, #16 * 28]\n"
                   "ldp    q30, q31, [%2, #16 * 30]\n"
                   "msr    fpcr, %0        \n"
                   "msr    fpsr, %1        \n"
                   :
                   : "r"(uctx->__fregs.__fpcr), "r"(uctx->__fregs.__fpsr),
                     "r"(uctx->__fregs.__qregs));
}
