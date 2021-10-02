/*
 * Test FPU context saving.
 */
#include <sys/klog.h>
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

static alignas(FPREGSIZE) uint8_t fpregs[_NFREG * FPREGSIZE];

#ifdef __mips__
#include <mips/m32c0.h>

/* clang-format off */
asm(".set noreorder\n\t"
    ".set hardfloat");

#define __enable_fpu()                                                         \
  __asm__ volatile("mfc0 $t0, $" DEF2STR(C0_STATUS) "\n\t"                     \
                   "li $t1, " DEF2STR(SR_CU1) "\n\t"                           \
                   "or $t0, $t1\n\t"                                           \
                   "mtc0 $t0, $" DEF2STR(C0_STATUS)                            \
                   ::: "$t0", "$t1")

#define __set_fp_reg(n)                                                        \
  __asm__ volatile("lw $t0, magic_value\n\t"                                   \
                   "mtc1 $t0, $f" #n                                           \
                   ::: "$t0")

#define __get_fp_reg(n)                                                        \
  __asm__ volatile("la $t0, fpregs\n\t"                                        \
                   "addu $t0, $t0, %[off]\n\t"                                 \
                   "swc1 $f" #n ", ($t0)"                                      \
		   :: [off] "r" (n * FPREGSIZE)                                \
                   : "$t0")

#define __disable_fpu()                                                        \
  __asm__ volatile("mfc0 $t0, $" DEF2STR(C0_STATUS) "\n\t"                     \
                   "li $t1, ~" DEF2STR(SR_CU1) "\n\t"                          \
                   "and $t0, $t1\n\t"                                          \
                   "mtc0 $t0, $" DEF2STR(C0_STATUS)                            \
                   ::: "$t0", "$t1")
/* clang-format on */

#define __sp()                                                                 \
  ({                                                                           \
    uint32_t __rv;                                                             \
    __asm __volatile("move %0, $sp" : "=r"(__rv));                             \
    __rv;                                                                      \
  })

#endif /* !__mips__ */

#ifdef __aarch64__
#include <aarch64/armreg.h>

/* clang-format off */
#define __enable_fpu()                                                         \
  __asm__ volatile("mrs x0, cpacr_el1\n\t"                                     \
                   "and x0, x0, ~" DEF2STR(CPACR_FPEN_MASK) "\n\t"             \
                   "msr cpacr_el1, x0"                                         \
                   ::: "x0")

#define __set_fp_reg(n)                                                        \
  __asm__ volatile("ldr x0, =magic_value\n\t"                                  \
                   "ldr q" #n ", [x0]"                                         \
                   ::: "x0", "q0")

#define __get_fp_reg(n)                                                        \
  __asm__ volatile("ldr x0, =fpregs\n\t"                                       \
                   "add x0, x0, %[off]\n\t"                                    \
                   "str q" #n ", [x0]"                                         \
                   :: [off] "r" (n * FPREGSIZE)                                \
                   : "x0")

#define __disable_fpu()                                                        \
  __asm__ volatile("mrs x0, cpacr_el1\n\t"                                     \
                   "and x0, x0, ~" DEF2STR(CPACR_FPEN_MASK) "\n\t"             \
                   "msr cpacr_el1, x0"                                         \
                   ::: "x0")
/* clang-format on */

#define __sp()                                                                 \
  ({                                                                           \
    uint64_t __rv;                                                             \
    __asm __volatile("mov %0, sp" : "=r"(__rv));                               \
    __rv;                                                                      \
  })

#endif /* !__aarch64__ */

static thread_t *switcher;
static bool fpu_ctx_set;
static spin_t fcs_lock;
static condvar_t fcs_cv;

static tdp_flags_t td_pflags;

static void set_fp_regs(void) {
  assert(preempt_disabled());

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
}

static void get_fp_regs(void) {
  assert(preempt_disabled());

  __get_fp_reg(0);
  __get_fp_reg(1);
  __get_fp_reg(2);
  __get_fp_reg(3);
  __get_fp_reg(4);
  __get_fp_reg(5);
  __get_fp_reg(6);
  __get_fp_reg(7);
  __get_fp_reg(8);
  __get_fp_reg(9);
  __get_fp_reg(10);
  __get_fp_reg(11);
  __get_fp_reg(12);
  __get_fp_reg(13);
  __get_fp_reg(14);
  __get_fp_reg(15);
  __get_fp_reg(16);
  __get_fp_reg(17);
  __get_fp_reg(18);
  __get_fp_reg(19);
  __get_fp_reg(20);
  __get_fp_reg(21);
  __get_fp_reg(22);
  __get_fp_reg(23);
  __get_fp_reg(24);
  __get_fp_reg(25);
  __get_fp_reg(26);
  __get_fp_reg(27);
  __get_fp_reg(28);
  __get_fp_reg(29);
  __get_fp_reg(30);
  __get_fp_reg(31);

  __disable_fpu();
}

static void switch_thread(void *p __unused) {
  /* Wait until the FPU contex of the test thread has been set. */
  WITH_SPIN_LOCK (&fcs_lock) {
    while (!fpu_ctx_set)
      cv_wait(&fcs_cv, &fcs_lock);
  }
}

static void epilog(void) {
  thread_t *td = thread_self();
  get_fp_regs();
  td_pflags = td->td_pflags;
  preempt_enable();
}

static void test_thread(void *p __unused) {
  thread_t *td = thread_self();

  /* Before ctx swtich. */
  WITH_NO_PREEMPTION {
    set_fp_regs();
    td->td_pflags |= TDP_FPUINUSE;
  }

  /* Signal to the notifier that the FPU context has been set
   * and we're ready for a context switch. */
  fpu_ctx_set = true;
  cv_signal(&fcs_cv);

  /* Wait for the switcher thread to finish
   * (it will ensure a context switch). */
  thread_join(switcher);

  /* After ctx switch. */
  WITH_NO_PREEMPTION {
    ctx_t *uctx = (ctx_t *)td->td_uctx;
    ctx_init(uctx, epilog, (void *)__sp());
    ctx_setup_call(uctx, (register_t)thread_exit, 0);
    user_exc_leave();
  }
}

static int test_fpu_ctx(void) {
  spin_init(&fcs_lock, 0);
  cv_init(&fcs_cv, "FPU context set");

  thread_t *td =
    thread_create("test-fp-ctx-0", test_thread, NULL, prio_kthread(0));
  switcher =
    thread_create("test-fp-ctx-1", switch_thread, NULL, prio_kthread(0));

  sched_add(td);
  sched_add(switcher);

  thread_join(td);

  /*
   * Let's see if the FP context has been saved and restored correctly.
   */
  int res = KTEST_FAILURE;

  if ((td_pflags & (TDP_FPUINUSE | TDP_FPUCTXSAVED)) != TDP_FPUINUSE)
    goto end;

  for (size_t i = 0; i < _NFREG; i++) {
    if (memcmp(&fpregs[i * FPREGSIZE], magic_value, FPREGSIZE))
      goto end;
  }
  res = KTEST_SUCCESS;

end:
  thread_reap();
  return res;
}

KTEST_ADD(fpu_ctx, test_fpu_ctx, 0);
