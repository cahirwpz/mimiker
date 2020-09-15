#include <sys/errno.h>
#include <sys/interrupt.h>
#include <mips/context.h>
#include <mips/interrupt.h>
#include <sys/pcpu.h>
#include <sys/pmap.h>
#include <sys/sysent.h>
#include <sys/thread.h>
#include <sys/ktest.h>

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

static void user_trap_handler(ctx_t *ctx) {
  /* We came here from user-space,
   * hence interrupts and preemption must have be enabled. */
  cpu_intr_enable();

  assert(!intr_disabled() && !preempt_disabled());

  int cp_id;

  switch (exc_code(ctx)) {
    case EXC_MOD:
    case EXC_TLBL:
    case EXC_TLBS:
      tlb_exception_handler(ctx);
      break;

    /*
     * An address error exception occurs under the following circumstances:
     *  - instruction address is not aligned on a word boundary
     *  - load/store with an address is not aligned on a word/halfword boundary
     *  - reference to a kernel/supervisor address from user-space
     */
    case EXC_ADEL:
    case EXC_ADES:
      sig_trap(ctx, SIGBUS);
      break;

    case EXC_SYS:
      syscall_handler(ctx);
      break;

    case EXC_FPE:
    case EXC_MSAFPE:
    case EXC_OVF:
      sig_trap(ctx, SIGFPE);
      break;

    case EXC_CPU:
      cp_id = (_REG(ctx, CAUSE) & CR_CEMASK) >> CR_CESHIFT;
      if (cp_id != 1) {
        sig_trap(ctx, SIGILL);
      } else {
        /* Enable FPU for interrupted context. */
        _REG(ctx, SR) |= SR_CU1;
      }
      break;

    case EXC_RI:
      sig_trap(ctx, SIGILL);
      break;

    default:
      kernel_oops(ctx);
  }

  /* This is right moment to check if out time slice expired. */
  on_exc_leave();

  /* If we're about to return to user mode then check pending signals, etc. */
  on_user_exc_leave();
}

static void kern_trap_handler(ctx_t *ctx) {
  /* We came here from kernel-space...  */
  PCPU_SET(no_switch, true);

  switch (exc_code(ctx)) {
    case EXC_MOD:
    case EXC_TLBL:
    case EXC_TLBS:
      tlb_exception_handler(ctx);
      break;

    default:
      kernel_oops(ctx);
  }

  PCPU_SET(no_switch, false);
}

void mips_exc_handler(ctx_t *ctx) {
  assert(cpu_intr_disabled());

  bool kern_mode = kern_mode_p(ctx);

  if (kern_mode) {
    /* If there's not enough space on the stack to store another exception
     * frame we consider situation to be critical and panic.
     * Hopefully sizeof(ctx_t) bytes of unallocated stack space will be enough
     * to display error message. */
    register const register_t sp asm("$sp");
    if ((sp & (PAGESIZE - 1)) < sizeof(ctx_t)) {
      kprintf("Kernel stack overflow caught at $%08lx!\n", _REG(ctx, EPC));
      if (ktest_test_running_flag)
        ktest_failure();
      else
        panic();
    }
  }

  if (exc_code(ctx)) {
    mips_intr_handler(ctx);
  } else {
    if (kern_mode)
      kern_trap_handler(ctx);
    else
      user_trap_handler(ctx);
  }
}
