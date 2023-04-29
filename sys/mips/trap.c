#define KL_LOG KL_INTR
#include <sys/klog.h>
#include <sys/interrupt.h>
#include <sys/cpu.h>
#include <sys/context.h>
#include <sys/pmap.h>
#include <sys/errno.h>
#include <sys/sched.h>
#include <sys/sysent.h>
#include <sys/thread.h>
#include <mips/mcontext.h>
#include <mips/tlb.h>

__no_profile static inline unsigned exc_code(ctx_t *ctx) {
  return (_REG(ctx, CAUSE) & CR_X_MASK) >> CR_X_SHIFT;
}

/*
 * This is exception name table. For exact meaning of exception handlers
 * numbers please check 5.23 Table of MIPS32 4KEc User's Manual.
 */

/* clang-format off */
static const char *const exceptions[32] = {
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

static __noreturn void kernel_oops(ctx_t *ctx) {
  unsigned code = exc_code(ctx);

  klog("%s at $%08lx!", exceptions[code], _REG(ctx, EPC));
  switch (code) {
    case EXC_ADEL:
    case EXC_ADES:
    case EXC_IBE:
    case EXC_DBE:
    case EXC_TLBL:
    case EXC_TLBS:
    case EXC_TLBRI:
    case EXC_TLBXI:
      klog("Caused by reference to $%08lx!", _REG(ctx, BADVADDR));
      break;
    case EXC_RI:
      klog("Reserved instruction exception in kernel mode!");
      break;
    case EXC_CPU:
      klog("Coprocessor unusable exception in kernel mode!");
      break;
    case EXC_FPE:
    case EXC_MSAFPE:
    case EXC_OVF:
      klog("Floating point exception or integer overflow in kernel mode!");
      break;
    default:
      break;
  }
  klog("HINT: Type 'info line *0x%08lx' into gdb to find faulty code line.",
       _REG(ctx, EPC));
  panic("KERNEL PANIC!!!");
}

static vm_prot_t exc_access(u_long code) {
  if (code == EXC_TLBL || code == EXC_TLBRI)
    return VM_PROT_READ;
  if (code == EXC_TLBS || code == EXC_MOD)
    return VM_PROT_WRITE;
  if (code == EXC_TLBXI)
    return VM_PROT_EXEC;
  panic("unknown code: %s", exceptions[code]);
}

static void user_trap_handler(ctx_t *ctx) {
  /* We came here from user-space,
   * hence interrupts and preemption must have be enabled. */
  cpu_intr_enable();

  assert(!intr_disabled() && !preempt_disabled());

  int cp_id, error;
  syscall_result_t result;
  unsigned int code = exc_code(ctx);
  vaddr_t vaddr = _REG(ctx, BADVADDR);
  vaddr_t epc = _REG(ctx, EPC);

  switch (code) {
    case EXC_MOD:
    case EXC_TLBL:
    case EXC_TLBS:
    case EXC_TLBRI:
    case EXC_TLBXI:
      klog("%s at $%lx, caused by reference to $%lx!", exceptions[code], epc,
           vaddr);
      if ((error = pmap_fault_handler(ctx, vaddr, exc_access(code))))
        sig_trap(SIGSEGV, error == EFAULT ? SEGV_MAPERR : SEGV_ACCERR,
                 (void *)vaddr, code);
      break;

    /*
     * An address error exception occurs under the following circumstances:
     *  - instruction address is not aligned on a word boundary
     *  - load/store with an address is not aligned on a word/halfword boundary
     *  - reference to a kernel/supervisor address from user-space
     */
    case EXC_ADEL:
    case EXC_ADES:
      sig_trap(SIGBUS, BUS_ADRALN, (void *)epc, code);
      break;

    case EXC_SYS:
      syscall_handler(_REG(ctx, V0), ctx, &result);
      break;

    case EXC_FPE:
    case EXC_MSAFPE:
    case EXC_OVF:
      sig_trap(SIGFPE, FPE_INTOVF, (void *)epc, code);
      break;

    case EXC_CPU:
      cp_id = (_REG(ctx, CAUSE) & CR_CEMASK) >> CR_CESHIFT;
      if (cp_id != 1) {
        sig_trap(SIGILL, ILL_ILLOPC, (void *)epc, code);
      } else {
        /* Enable FPU for interrupted context. */
        thread_self()->td_pflags |= TDP_FPUINUSE;
        _REG(ctx, SR) |= SR_CU1;
      }
      break;

    case EXC_RI:
      sig_trap(SIGILL, ILL_PRVOPC, (void *)epc, code);
      break;

    default:
      kernel_oops(ctx);
  }

  /* This is right moment to check if out time slice expired. */
  sched_maybe_preempt();

  /* If we're about to return to user mode then check pending signals, etc. */
  sig_userret((mcontext_t *)ctx, code == EXC_SYS ? &result : NULL);
}

static void kern_trap_handler(ctx_t *ctx) {
  /* We came here from kernel-space. If interrupts were enabled before we
   * trapped, then turn them on here. */
  if (_REG(ctx, SR) & SR_IE)
    cpu_intr_enable();

  u_long code = exc_code(ctx);
  vaddr_t vaddr = _REG(ctx, BADVADDR);

  switch (code) {
    case EXC_MOD:
    case EXC_TLBL:
    case EXC_TLBS:
      klog("%s at $%08x, caused by reference to $%08lx!", exceptions[code],
           _REG(ctx, EPC), vaddr);
      if (pmap_fault_handler(ctx, vaddr, exc_access(code)))
        kernel_oops(ctx);
      break;

    default:
      kernel_oops(ctx);
  }
}

__no_profile void mips_exc_handler(ctx_t *ctx) {
  thread_t *td = thread_self();
  assert(td->td_idnest == 0);
  assert(cpu_intr_disabled());

  bool user_mode = user_mode_p(ctx);
  ctx_t *kframe_saved;

  if (!user_mode) {
    /* If there's not enough space on the stack to store another exception
     * frame we consider situation to be critical and panic.
     * Hopefully sizeof(ctx_t) bytes of unallocated stack space will be enough
     * to display error message. */
    register_t sp = mips32_get_sp();
    if ((sp & (PAGESIZE - 1)) < sizeof(ctx_t))
      panic("Kernel stack overflow caught at $%08lx!", _REG(ctx, EPC));
    kframe_saved = td->td_kframe;
    td->td_kframe = ctx;
  }

  if (exc_code(ctx)) {
    if (user_mode)
      user_trap_handler(ctx);
    else
      kern_trap_handler(ctx);
  } else {
    intr_root_handler(ctx);
  }

  if (!user_mode)
    td->td_kframe = kframe_saved;
}
