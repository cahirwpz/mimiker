/*
 * Test FPU context saving.
 */
#include <sys/cdefs.h>
#include <sys/ktest.h>
#include <sys/sched.h>
#include <sys/thread.h>
#include <machine/mcontext.h>

#define DEF2STR(x) __STRING(x)
#define _MACHDEP

#ifdef __mips__
#define FPREGSIZE 4
#define FPREGOFF offsetof(mcontext_t, __fpregs)
#endif /* !__mips__ */

#ifdef __aarch64__
#define FPREGSIZE 16
#define FPREGOFF offsetof(mcontext_t, __fregs)
#endif /* !__aarch64__ */

static alignas(FPREGSIZE) uint8_t magic_value[FPREGSIZE] = {
  0x01, 0x23, 0x45, 0x67,
#if FPREGSIZE == 8
  0x89, 0xab, 0xcd, 0xef,
#if FPREGSIZE == 16
  0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef,
#endif /* !FPREGSIZE == 16 */
#endif /* !FPREGSIZE == 8 */
};

static_assert(_NFREG == 32, "we assume there are 32 FP registers!");

#ifdef __mips__
#include <mips/m32c0.h>

/* clang-format off */
asm(".set noreorder\n\t"
    ".set hardfloat");

#define __enable_fpu()                                                         \
  __asm__ volatile("mfc0 $t0, $" DEF2STR(C0_STATUS) "\n\t"                     \
                   "li $t1, " DEF2STR(SR_CU1) "\n\t"                           \
                   "or $t0, $t1\n\t"                                           \
                   "mtc0 $t0, $" DEF2STR(C0_STATUS))

#define __set_fp_reg(n)                                                        \
  __asm__ volatile("lw $t0, magic_value\n\t"                                   \
                   "mtc1 $t0, $f" #n)

#define __disable_fpu()                                                        \
  __asm__ volatile("mfc0 $t0, $" DEF2STR(C0_STATUS) "\n\t"                     \
                   "li $t1, ~" DEF2STR(SR_CU1) "\n\t"                          \
                   "and $t0, $t1\n\t"                                          \
                   "mtc0 $t0, $" DEF2STR(C0_STATUS))
/* clang-format on */
#endif /* !__mips__ */

#ifdef __aarch64__
#include <aarch64/armreg.h>

/* clang-format off */
#define __enable_fpu()                                                         \
  __asm__ volatile("mrs x0, cpacr_el1\n\t"                                     \
                   "and x0, x0, ~" DEF2STR(CPACR_FPEN_MASK) "\n\t"             \
                   "msr cpacr_el1, x0")

#define __set_fp_reg(n)                                                        \
  __asm__ volatile("ldr x0, =magic_value\n\t"                                  \
                   "ldr q" #n ", [x0]")

#define __disable_fpu()                                                        \
  __asm__ volatile("mrs x0, cpacr_el1\n\t"                                     \
                   "and x0, x0, ~" DEF2STR(CPACR_FPEN_MASK) "\n\t"             \
                   "msr cpacr_el1, x0")
/* clang-format on */
#endif /* !__aarch64__ */

static void set_fp_regs(void) {
  __enable_fpu();

  __set_fp_reg(0);
  __set_fp_reg(1);
  __set_fp_reg(2);
  __set_fp_reg(3);
  __set_fp_reg(4);
  __set_fp_reg(5);
  __set_fp_reg(6);
  __set_fp_reg(7);
  __set_fp_reg(8);
  __set_fp_reg(9);
  __set_fp_reg(10);
  __set_fp_reg(11);
  __set_fp_reg(12);
  __set_fp_reg(13);
  __set_fp_reg(14);
  __set_fp_reg(15);
  __set_fp_reg(16);
  __set_fp_reg(17);
  __set_fp_reg(18);
  __set_fp_reg(19);
  __set_fp_reg(20);
  __set_fp_reg(21);
  __set_fp_reg(22);
  __set_fp_reg(23);
  __set_fp_reg(24);
  __set_fp_reg(25);
  __set_fp_reg(26);
  __set_fp_reg(27);
  __set_fp_reg(28);
  __set_fp_reg(29);
  __set_fp_reg(30);
  __set_fp_reg(31);

  __disable_fpu();
}

static void test_thread(void *p __unused) {
  WITH_NO_PREEMPTION {
    set_fp_regs();

    thread_t *td = thread_self();
    td->td_pflags |= TDP_FPUINUSE;
  }
  thread_yield();
}

static int test_fpu_ctx(void) {
  thread_t *td =
    thread_create("test-fp-ctx-0", test_thread, NULL, prio_kthread(0));

  sched_add(td);
  thread_join(td);

  /*
   * Let's see if the FP context has been saved.
   */
  int res = KTEST_FAILURE;

  if (!(td->td_pflags & TDP_FPUCTXSAVED))
    goto end;

  mcontext_t *uctx = td->td_uctx;
  void *fp_regs = (void *)uctx + FPREGOFF;

  for (size_t i = 0; i < _NFREG; i++, fp_regs += FPREGSIZE) {
    if (memcmp(fp_regs, magic_value, FPREGSIZE))
      goto end;
  }
  res = KTEST_SUCCESS;

end:
  thread_reap();
  return res;
}

KTEST_ADD(fpu_ctx, test_fpu_ctx, 0);
