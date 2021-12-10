#define KL_LOG KL_VM
#include <sys/context.h>
#include <sys/cpu.h>
#include <sys/errno.h>
#include <sys/interrupt.h>
#include <sys/klog.h>
#include <sys/pmap.h>
#include <sys/sysent.h>
#include <sys/thread.h>
#include <sys/vm_map.h>
#include <riscv/mcontext.h>
#include <riscv/riscvreg.h>

#define __sp()                                                                 \
  ({                                                                           \
    uint64_t __rv;                                                             \
    __asm __volatile("mv %0, sp" : "=r"(__rv));                                \
    __rv;                                                                      \
  })

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

__no_profile static inline unsigned ctx_code(ctx_t *ctx) {
  return _REG(ctx, CAUSE) & SCAUSE_CODE;
}

__no_profile static inline unsigned ctx_intr_enabled(ctx_t *ctx) {
  return _REG(ctx, SR) & SSTATUS_SPIE;
}

static __noreturn void kernel_oops(ctx_t *ctx) {
  unsigned code = ctx_code(ctx);
  void *epc = (void *)_REG(ctx, PC);

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
      uint32_t badinstr = *(uint32_t *)epc;
      klog("Illegal instruction %08x in kernel mode!", badinstr);
      break;
    case SCAUSE_BREAKPOINT:
      klog("No debbuger in kernel!");
      break;
  }
  klog("HINT: Type 'info line *%p' into gdb to find faulty code line", epc);

  panic("KERNEL PANIC!!!");
}

static void page_fault_handler(ctx_t *ctx) {
  thread_t *td = thread_self();

  unsigned code = ctx_code(ctx);
  void *epc = (void *)_REG(ctx, PC);
  vaddr_t vaddr = _REG(ctx, TVAL);

  klog("%s at %p, caused by reference to %lx!", exceptions[code], epc, vaddr);

  pmap_t *pmap = pmap_lookup(vaddr);
  if (!pmap) {
    klog("No physical map defined for %lx address!", vaddr);
    goto fault;
  }

  vm_prot_t access;
  if (code == SCAUSE_INST_PAGE_FAULT)
    access = VM_PROT_EXEC;
  else if (code == SCAUSE_LOAD_PAGE_FAULT)
    access = VM_PROT_READ;
  else
    access = VM_PROT_WRITE;

  int error = pmap_emulate_bits(pmap, vaddr, access);
  if (!error)
    return;

  if (error == EACCES || error == EINVAL)
    goto fault;

  vm_map_t *vmap = vm_map_lookup(vaddr);
  if (!vmap) {
    klog("No virtual address space defined for %lx!", vaddr);
    goto fault;
  }

  if (!vm_page_fault(vmap, vaddr, access))
    return;

fault:
  if (td->td_onfault) {
    /* Handle copyin/copyout faults. */
    _REG(ctx, PC) = td->td_onfault;
    td->td_onfault = 0;
  } else if (user_mode_p(ctx)) {
    /* Send a segmentation fault signal to the user program. */
    sig_trap(ctx, SIGSEGV);
  } else {
    /* Panic when kernel-mode thread uses wrong pointer. */
    kernel_oops(ctx);
  }
}

/*
 * RISC-V syscall ABI:
 *  - a7: code
 *  - a0-5: args
 *
 * NOTE: the following code assumes all arguments to syscalls are passed
 * via registers.
 */
static_assert(SYS_MAXSYSARGS <= FUNC_MAXREGARGS - 1,
              "Syscall args don't fit in registers!");

static void syscall_handler(mcontext_t *uctx, syscall_result_t *result) {
  /* TODO: eventually we should have a platform-independent syscall handler. */
  register_t args[SYS_MAXSYSARGS];
  register_t code = _REG(uctx, A7);

  memcpy(args, &_REG(uctx, A0), sizeof(args));

  if (code > SYS_MAXSYSCALL) {
    args[0] = code;
    code = 0;
  }

  sysent_t *se = &sysent[code];
  size_t nargs = se->nargs;

  assert(nargs <= SYS_MAXSYSCALL);

  thread_t *td = thread_self();
  register_t retval = 0;

  assert(td->td_proc);

  int error = se->call(td->td_proc, (void *)args, &retval);

  result->retval = error ? -1 : retval;
  result->error = error;
}

static void user_trap_handler(ctx_t *ctx) {
  /*
   * We came here from user-space, hence interrupts and preemption must
   * have been enabled.
   */
  cpu_intr_enable();

  assert(!intr_disabled() && !preempt_disabled());

  syscall_result_t result;
  unsigned code = ctx_code(ctx);

  switch (code) {
    case SCAUSE_INST_PAGE_FAULT:
    case SCAUSE_LOAD_PAGE_FAULT:
    case SCAUSE_STORE_PAGE_FAULT:
      page_fault_handler(ctx);
      break;

      /* Access fault */
    case SCAUSE_INST_ACCESS_FAULT:
    case SCAUSE_LOAD_ACCESS_FAULT:
    case SCAUSE_STORE_ACCESS_FAULT:
      /* Missaligned access */
    case SCAUSE_INST_MISALIGNED:
    case SCAUSE_LOAD_MISALIGNED:
    case SCAUSE_STORE_MISALIGNED:
      sig_trap(ctx, SIGBUS);
      break;

    case SCAUSE_ECALL_USER:
      syscall_handler((mcontext_t *)ctx, &result);
      break;

    case SCAUSE_ILLEGAL_INSTRUCTION:
      /* TODO(MichalBlk): enable FPE if requested. */
      sig_trap(ctx, SIGILL);
      break;

    case SCAUSE_BREAKPOINT:
      sig_trap(ctx, SIGTRAP);
      break;

    default:
      kernel_oops(ctx);
  }

  /* This is a right moment to check if our time slice expired. */
  on_exc_leave();

  /* If we're about to return to user mode, then check pending signals, etc. */
  on_user_exc_leave((mcontext_t *)ctx,
                    code == SCAUSE_ECALL_USER ? &result : NULL);
}

static void kern_trap_handler(ctx_t *ctx) {
  /*
   * We came here from kernel-space. If interrupts were enabled before we
   * trapped, then turn them on here.
   */
  if (ctx_intr_enabled(ctx))
    cpu_intr_enable();

  switch (ctx_code(ctx)) {
    case SCAUSE_INST_PAGE_FAULT:
    case SCAUSE_LOAD_PAGE_FAULT:
    case SCAUSE_STORE_PAGE_FAULT:
      page_fault_handler(ctx);
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

  if (!user_mode) {
    /* If there's not enough space on the stack to store another exception
     * frame we consider situation to be critical and panic.
     * Hopefully sizeof(ctx_t) bytes of unallocated stack space will be enough
     * to display error message. */
    register_t sp = __sp();
    if ((sp & (PAGESIZE - 1)) < sizeof(ctx_t))
      panic("Kernel stack overflow caught at %p!", _REG(ctx, PC));
  }

  if (ctx_interrupt(ctx)) {
    intr_root_handler(ctx);
  } else {
    if (user_mode)
      user_trap_handler(ctx);
    else
      kern_trap_handler(ctx);
  }
}
