#define KL_LOG KL_INTR
#include <sys/klog.h>
#include <sys/cpu.h>
#include <sys/errno.h>
#include <sys/interrupt.h>
#include <sys/pmap.h>
#include <sys/sysent.h>
#include <sys/thread.h>
#include <sys/sched.h>
#include <riscv/cpufunc.h>

/* clang-format off */
static const char *const exceptions[] = {
  [SCAUSE_INST_MISALIGNED] = "Misaligned instruction",
  [SCAUSE_INST_ACCESS_FAULT] = "Instruction access fault",
  [SCAUSE_ILLEGAL_INSTRUCTION] = "Illegal instruction",
  [SCAUSE_BREAKPOINT] = "Breakpoint",
  [SCAUSE_LOAD_MISALIGNED] = "Misaligned load",
  [SCAUSE_LOAD_ACCESS_FAULT] = "Load access fault",
  [SCAUSE_STORE_MISALIGNED] = "Misaligned store",
  [SCAUSE_STORE_ACCESS_FAULT] = "Store access fault",
  [SCAUSE_ECALL_USER] = "User environment call",
  [SCAUSE_ECALL_SUPERVISOR] = "Supervisor environment call",
  [SCAUSE_INST_PAGE_FAULT] = "Instruction page fault",
  [SCAUSE_LOAD_PAGE_FAULT] = "Load page fault",
  [SCAUSE_STORE_PAGE_FAULT] = "Store page fault",
};
/* clang-format on */

__no_profile static inline bool ctx_interrupt(ctx_t *ctx) {
  return _REG(ctx, CAUSE) & SCAUSE_INTR;
}

__no_profile static inline u_long ctx_code(ctx_t *ctx) {
  return _REG(ctx, CAUSE) & SCAUSE_CODE;
}

__no_profile static inline bool ctx_intr_enabled(ctx_t *ctx) {
  return _REG(ctx, SR) & SSTATUS_SPIE;
}

static __noreturn void kernel_oops(ctx_t *ctx) {
  u_long code = ctx_code(ctx);
  void *epc = (void *)_REG(ctx, PC);
  uint32_t badinstr = *(uint32_t *)epc;

  klog("%s at %p!", exceptions[code], epc);

  switch (code) {
    case SCAUSE_INST_MISALIGNED:
    case SCAUSE_INST_ACCESS_FAULT:
    case SCAUSE_LOAD_MISALIGNED:
    case SCAUSE_LOAD_ACCESS_FAULT:
    case SCAUSE_STORE_MISALIGNED:
    case SCAUSE_STORE_ACCESS_FAULT:
    case SCAUSE_INST_PAGE_FAULT:
    case SCAUSE_LOAD_PAGE_FAULT:
    case SCAUSE_STORE_PAGE_FAULT:
      klog("Caused by reference to %lx!", _REG(ctx, TVAL));
      break;

    case SCAUSE_ILLEGAL_INSTRUCTION:
      klog("Illegal instruction %08x in kernel mode!", badinstr);
      break;

    case SCAUSE_BREAKPOINT:
      klog("No debbuger in kernel!");
      break;
  }
  klog("HINT: Type 'info line *%p' into gdb to find faulty code line", epc);

  panic("KERNEL PANIC!!!");
}

/*
 * NOTE: for each thread, dirty FPE context has to be saved
 * during each ctx switch. To decrease the cost of this procedure
 * we disable FPU for each new thread and enable it only when actually
 * requested by the thread. Such request resolves to an illegal instruction
 * exception. Thereby, each time an illegal instruction exception occurs,
 * we check wheter it is an FPU request issued by the thread
 * or an unhanled opcode.
 * The check and FPU enablement is performed by `fpu_handler`.
 */
static bool fpu_handler(mcontext_t *uctx) {
  if (!FPU)
    return false;

  thread_t *td = thread_self();

  if (td->td_pflags & TDP_FPUINUSE)
    return false;

  /*
   * May be an FPE trap. Enable FPE usage
   * for this thread and try again.
   */
  memset(uctx->__fregs, 0, sizeof(__fregset_t));
  _REG(uctx, SR) &= ~SSTATUS_FS_MASK;
  _REG(uctx, SR) |= SSTATUS_FS_CLEAN;
  td->td_pflags |= TDP_FPUINUSE;

  return true;
}

static vm_prot_t exc_access(u_long exc_code) {
  switch (exc_code) {
    case SCAUSE_INST_PAGE_FAULT:
      return VM_PROT_EXEC;
    case SCAUSE_LOAD_PAGE_FAULT:
      return VM_PROT_READ;
    default:
      return VM_PROT_WRITE;
  }
}

static void user_trap_handler(ctx_t *ctx) {
  /*
   * We came here from user-space, hence interrupts and preemption must
   * have been enabled.
   */
  cpu_intr_enable();

  assert(!intr_disabled() && !preempt_disabled());

  syscall_result_t result;
  u_long code = ctx_code(ctx);
  void *epc = (void *)_REG(ctx, PC);
  vaddr_t vaddr = _REG(ctx, TVAL);

  switch (code) {
    case SCAUSE_INST_PAGE_FAULT:
    case SCAUSE_LOAD_PAGE_FAULT:
    case SCAUSE_STORE_PAGE_FAULT:
      klog("%s at %p, caused by reference to %lx!", exceptions[code], epc,
           vaddr);
      int signo = 0, sigcode;
      int err = pmap_fault_handler(ctx, vaddr, exc_access(code));

      if (err == EACCES) {
        signo = SIGSEGV;
        sigcode = SEGV_ACCERR;
      } else if (err == EFAULT) {
        signo = SIGSEGV;
        sigcode = SEGV_MAPERR;
      } else if (err > 0) {
        panic("Unknown error returned from abort handler: %d", err);
      }

      if (err) {
        sig_trap(ctx, signo, sigcode, (void *)vaddr, code);
      }

      break;

      /* Access fault */
    case SCAUSE_INST_ACCESS_FAULT:
    case SCAUSE_LOAD_ACCESS_FAULT:
    case SCAUSE_STORE_ACCESS_FAULT:
      /* Missaligned access */
    case SCAUSE_INST_MISALIGNED:
    case SCAUSE_LOAD_MISALIGNED:
    case SCAUSE_STORE_MISALIGNED:
      sig_trap(ctx, SIGBUS, BUS_ADRALN, (void *)vaddr, code);
      break;

    case SCAUSE_ECALL_USER:
      syscall_handler(_REG(ctx, A7), ctx, &result);
      break;

    case SCAUSE_ILLEGAL_INSTRUCTION:
      if (fpu_handler((mcontext_t *)ctx))
        break;
      klog("%s at %p!", exceptions[code], epc);
      sig_trap(ctx, SIGILL, ILL_ILLOPC, (void *)vaddr, code);
      break;

    case SCAUSE_BREAKPOINT:
      sig_trap(ctx, SIGTRAP, TRAP_BRKPT, (void *)vaddr, code);
      break;

    default:
      kernel_oops(ctx);
  }

  /* This is a right moment to check if our time slice expired. */
  sched_maybe_preempt();

  /* If we're about to return to user mode, then check pending signals, etc. */
  sig_userret((mcontext_t *)ctx, code == SCAUSE_ECALL_USER ? &result : NULL);
}

static void kern_trap_handler(ctx_t *ctx) {
  /*
   * We came here from kernel-space. If interrupts were enabled before we
   * trapped, then turn them on here.
   */
  if (ctx_intr_enabled(ctx))
    cpu_intr_enable();

  u_long code = ctx_code(ctx);
  void *epc = (void *)_REG(ctx, PC);
  vaddr_t vaddr = _REG(ctx, TVAL);

  switch (ctx_code(ctx)) {
    case SCAUSE_INST_PAGE_FAULT:
    case SCAUSE_LOAD_PAGE_FAULT:
    case SCAUSE_STORE_PAGE_FAULT:
      klog("%s at %p, caused by reference to %lx!", exceptions[code], epc,
           vaddr);
      if (pmap_fault_handler(ctx, vaddr, exc_access(code)))
        kernel_oops(ctx);
      break;

    default:
      kernel_oops(ctx);
  }
}

__no_profile void trap_handler(ctx_t *ctx) {
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
    uintptr_t sp = __sp();
    if (sp < (uintptr_t)td->td_kstack.stk_base + sizeof(ctx_t))
      panic("Kernel stack overflow caught at %p!", _REG(ctx, PC));
    kframe_saved = td->td_kframe;
    td->td_kframe = ctx;
  }

  if (ctx_interrupt(ctx)) {
    intr_root_handler(ctx);
  } else {
    if (user_mode)
      user_trap_handler(ctx);
    else
      kern_trap_handler(ctx);
  }

  if (!user_mode)
    td->td_kframe = kframe_saved;
}
