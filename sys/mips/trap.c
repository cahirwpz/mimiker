#include <sys/errno.h>
#include <sys/interrupt.h>
#include <mips/context.h>
#include <mips/interrupt.h>
#include <sys/pcpu.h>
#include <sys/pmap.h>
#include <sys/sysent.h>
#include <sys/thread.h>
#include <sys/ktest.h>

typedef void (*exc_handler_t)(ctx_t *);

static void syscall_handler(ctx_t *ctx) {
  /* TODO Eventually we should have a platform-independent syscall handler. */
  register_t args[SYS_MAXSYSARGS];
  register_t code = _REG(ctx, V0);
  const int nregs = 4;
  int error = 0;

  /*
   * Copy the arguments passed via register from the
   * trapctx to our argument array
   */
  memcpy(args, &_REG(ctx, A0), nregs * sizeof(register_t));

  if (code > SYS_MAXSYSCALL) {
    args[0] = code;
    code = 0;
  }

  sysent_t *se = &sysent[code];
  size_t nargs = se->nargs;

  if (nargs > nregs) {
    /*
     * From ABI:
     * Despite the fact that some or all of the arguments to a function are
     * passed in registers, always allocate space on the stack for all
     * arguments.
     * For this reason, we read from the user stack with some offset.
     */
    vaddr_t usp = _REG(ctx, SP) + nregs * sizeof(register_t);
    error = copyin((register_t *)usp, &args[nregs],
                   (nargs - nregs) * sizeof(register_t));
  }

  /* Call the handler. */
  thread_t *td = thread_self();
  register_t retval = 0;

  assert(td->td_proc != NULL);

  if (!error)
    error = se->call(td->td_proc, (void *)args, &retval);

  if (error != EJUSTRETURN)
    user_ctx_set_retval((user_ctx_t *)ctx, error ? -1 : retval, error);
}

static void fpe_handler(ctx_t *ctx) {
  if (kern_mode_p(ctx))
    kernel_oops(ctx);

  sig_trap(ctx, SIGFPE);
}

static void cpu_handler(ctx_t *ctx) {
  if (kern_mode_p(ctx))
    kernel_oops(ctx);

  int cp_id = (_REG(ctx, CAUSE) & CR_CEMASK) >> CR_CESHIFT;
  if (cp_id != 1) {
    sig_trap(ctx, SIGILL);
  } else {
    /* Enable FPU for interrupted context. */
    _REG(ctx, SR) |= SR_CU1;
  }
}

static void ri_handler(ctx_t *ctx) {
  if (kern_mode_p(ctx))
    kernel_oops(ctx);

  sig_trap(ctx, SIGILL);
}

/*
 * An address error exception occurs under the following circumstances:
 *  - instruction address is not aligned on a word boundary
 *  - load/store with an address is not aligned on a word/halfword boundary
 *  - reference to a kernel/supervisor address from user
 */
static void ade_handler(ctx_t *ctx) {
  if (kern_mode_p(ctx))
    kernel_oops(ctx);

  sig_trap(ctx, SIGBUS);
}

/*
 * This is exception vector table. Each exeception either has been assigned a
 * handler or kernel_oops is called for it. For exact meaning of exception
 * handlers numbers please check 5.23 Table of MIPS32 4KEc User's Manual.
 */

/* clang-format off */
const char *const exceptions[32] = {
  [EXC_INTR] = "Interrupt",
  [EXC_MOD] = "TLB modification exception",
  [EXC_TLBL] = "TLB exception (load or instruction fetch)",
  [EXC_TLBS] = "TLB exception (store)",
  [EXC_ADEL] = "Address error exception (load or instruction fetch)",
  [EXC_ADES] = "Address error exception (store)",
  [EXC_IBE] = "Bus error exception (instruction fetch)",
  [EXC_DBE] = "Bus error exception (data reference: load or store)",
  [EXC_SYS] = "System call",
  [EXC_BP] = "Breakpoint exception",
  [EXC_RI] = "Reserved instruction exception",
  [EXC_CPU] = "Coprocessor Unusable exception",
  [EXC_OVF] = "Arithmetic Overflow exception",
  [EXC_TRAP] = "Trap exception",
  [EXC_FPE] = "Floating point exception",
  [EXC_TLBRI] = "TLB read inhibit",
  [EXC_TLBXI] = "TLB execute inhibit",
  [EXC_WATCH] = "Reference to watchpoint address",
  [EXC_MCHECK] = "Machine checkcore",
};

static exc_handler_t exception_switch_table[32] = {
  [EXC_INTR] = mips_intr_handler,
  [EXC_MOD] = tlb_exception_handler,
  [EXC_TLBL] = tlb_exception_handler,
  [EXC_TLBS] = tlb_exception_handler,
  [EXC_ADEL] = ade_handler,
  [EXC_ADES] = ade_handler,
  [EXC_SYS] = syscall_handler,
  [EXC_FPE] = fpe_handler,
  [EXC_MSAFPE] = fpe_handler,
  [EXC_OVF] = fpe_handler,
  [EXC_CPU] = cpu_handler,
  [EXC_RI] = ri_handler
};
/* clang-format on */

__noreturn void kernel_oops(ctx_t *ctx) {
  unsigned code = exc_code(ctx);

  kprintf("KERNEL PANIC!!! \n");
  kprintf("%s at $%08lx!\n", exceptions[code], _REG(ctx, EPC));
  switch (code) {
    case EXC_ADEL:
    case EXC_ADES:
    case EXC_IBE:
    case EXC_DBE:
    case EXC_TLBL:
    case EXC_TLBS:
      kprintf("Caused by reference to $%08lx!\n", _REG(ctx, BADVADDR));
      break;
    case EXC_RI:
      kprintf("Reserved instruction exception in kernel mode!\n");
      break;
    case EXC_CPU:
      kprintf("Coprocessor unusable exception in kernel mode!\n");
      break;
    case EXC_FPE:
    case EXC_MSAFPE:
    case EXC_OVF:
      kprintf("Floating point exception or integer overflow in kernel mode!\n");
      break;
    default:
      break;
  }
  kprintf(
    "HINT: Type 'info line *0x%08lx' into gdb to find faulty code line.\n",
    _REG(ctx, EPC));
  if (ktest_test_running_flag)
    ktest_failure();
  else
    panic();
}

void kstack_overflow_handler(ctx_t *ctx) {
  kprintf("Kernel stack overflow caught at $%08lx!\n", _REG(ctx, EPC));
  if (ktest_test_running_flag)
    ktest_failure();
  else
    panic();
}

/* Let's consider possible contexts that caused exception/interrupt:
 *
 * 1. We came from user-space:
 *    interrupts and preemption must have been enabled
 * 2. We came from kernel-space:
 *    a. interrupts and preemption are enabled:
 *       kernel was running in regular thread context
 *    b. preemption is disabled, interrupts are enabled:
 *       kernel was running in thread context and preemption was disabled
 *       (this can happen during acquring or releasing sleep lock aka mutex)
 *    c. interrupts are disabled, preemption is implicitly disabled:
 *       kernel was running in interrupt context or acquired spin lock
 *
 * For each context we have a set of actions to be performed:
 *
 * 1. Handle interrupt with interrupts disabled or handle exception with
 *    interrupts and preemption enabled. Then check if user thread should be
 *    preempted. Finally prepare for return to user-space, for instance
 *    deliver signal to the process.
 * 2a. Same story as for (1) except we do not return to user-space.
 * 2b. Same as (2a) but do not check if the thread should be preempted.
 * 2c. Same as (2b) but do not enable interrupts for exception handling period.
 *
 * IMPORTANT! We should never call ctx_switch while interrupt is being handled
 *            or preemption is disabled.
 */
void mips_exc_handler(ctx_t *ctx) {
  unsigned code = exc_code(ctx);
  bool user_mode = user_mode_p(ctx);

  assert(intr_disabled());

  exc_handler_t handler = exception_switch_table[code];

  if (!handler)
    kernel_oops(ctx);

  if (!user_mode && (!(_REG(ctx, SR) & SR_IE) || preempt_disabled())) {
    /* We came here from kernel-space because of:
     * Case 2c: an exception, interrupts were disabled;
     * Case 2b: either interrupt or exception, preemption was disabled.
     * To prevent breaking critical section switching out is forbidden!
     *
     * In theory we can enable interrupts for the time of handling exception
     * in case 2b. In most cases handling exceptions in critical sections
     * will end up in kernel panic, since such scenario is usually caused
     * by programmer's error.
     *
     * XXX Being a very peculiar scenario we leave it as is for later
     *     consideration. Disabled interrupt will ease debugging.
     */
    PCPU_SET(no_switch, true);
    (*handler)(ctx);
    PCPU_SET(no_switch, false);
  } else {
    /* Case 1 & 2a: we came from user-space or kernel-space,
     * interrupts and preemption were enabled! */
    assert(_REG(ctx, SR) & SR_IE);
    assert(!preempt_disabled());

    if (code != EXC_INTR) {
      /* We assume it is safe to handle an exception with interrupts enabled. */
      intr_enable();
      (*handler)(ctx);
    } else {
      /* Switching out while handling interrupt is forbidden! */
      PCPU_SET(no_switch, true);
      (*handler)(ctx);
      PCPU_SET(no_switch, false);
      /* We did the job, so we don't need interrupts to be disabled anymore. */
      intr_enable();
    }

    /* This is right moment to check if out time slice expired. */
    on_exc_leave();

    /* If we're about to return to user mode then check pending signals, etc. */
    if (user_mode)
      on_user_exc_leave();

    /* Disable interrupts for the time interrupted context is being restored. */
    intr_disable();
  }

  assert(intr_disabled());
}
