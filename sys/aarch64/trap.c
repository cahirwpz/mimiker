#define KL_LOG KL_INTR
#include <sys/klog.h>
#include <sys/mimiker.h>
#include <sys/thread.h>
#include <sys/pmap.h>
#include <sys/sysent.h>
#include <sys/interrupt.h>
#include <sys/cpu.h>
#include <aarch64/armreg.h>

static __noreturn void kernel_oops(ctx_t *ctx) {
  panic("KERNEL PANIC!!!");
}

static vm_prot_t exc_access(u_long exc_code, register_t esr) {
  vm_prot_t access = VM_PROT_READ;
  if (exc_code == EXCP_INSN_ABORT || exc_code == EXCP_INSN_ABORT_L) {
    access |= VM_PROT_EXEC;
  } else if (esr & ISS_DATA_WnR) {
    access |= VM_PROT_WRITE;
  }
  return access;
}

static int unaligned_handler(ctx_t *ctx, vaddr_t vaddr, vm_prot_t access) {
  sig_trap(ctx, SIGBUS);
  return 0;
}

typedef int abort_handler_t(ctx_t *ctx, vaddr_t vaddr, vm_prot_t access);

static abort_handler_t *abort_handlers[ISS_DATA_DFSC_MASK + 1] = {
  [ISS_DATA_DFSC_TF_L0] = pmap_fault_handler,
  [ISS_DATA_DFSC_TF_L1] = pmap_fault_handler,
  [ISS_DATA_DFSC_TF_L2] = pmap_fault_handler,
  [ISS_DATA_DFSC_TF_L3] = pmap_fault_handler,
  [ISS_DATA_DFSC_AFF_L1] = pmap_fault_handler,
  [ISS_DATA_DFSC_AFF_L2] = pmap_fault_handler,
  [ISS_DATA_DFSC_AFF_L3] = pmap_fault_handler,
  [ISS_DATA_DFSC_PF_L1] = pmap_fault_handler,
  [ISS_DATA_DFSC_PF_L2] = pmap_fault_handler,
  [ISS_DATA_DFSC_PF_L3] = pmap_fault_handler,
  [ISS_DATA_DFSC_ALIGN] = unaligned_handler,
};

void user_trap_handler(mcontext_t *uctx) {
  /* Let's read special registers before enabling interrupts.
   * This ensures their values will not be lost. */
  ctx_t *ctx = (ctx_t *)uctx;
  register_t esr = READ_SPECIALREG(esr_el1);
  register_t far = READ_SPECIALREG(far_el1);
  syscall_result_t result;
  register_t exc_code = ESR_ELx_EXCEPTION(esr);
  register_t dfsc = esr & ISS_DATA_DFSC_MASK;

  cpu_intr_enable();

  assert(!intr_disabled() && !preempt_disabled());

  switch (exc_code) {
    case EXCP_INSN_ABORT_L:
    case EXCP_INSN_ABORT:
    case EXCP_DATA_ABORT_L:
    case EXCP_DATA_ABORT:
      if (abort_handlers[dfsc]) {
        abort_handlers[dfsc](ctx, far, exc_access(exc_code, esr));
      } else {
        panic("Unhandled EL0 %s abort (0x%x) at %p caused by reference to %p!",
              exc_code == EXCP_INSN_ABORT_L ? "instruction" : "data", dfsc,
              _REG(ctx, PC), far);
      }
      break;

    case EXCP_SVC64:
      syscall_handler(ESR_ELx_SYSCALL(esr), ctx, &result);
      break;

    case EXCP_SP_ALIGN:
    case EXCP_PC_ALIGN:
      unaligned_handler(ctx, far, exc_access(exc_code, esr));
      break;

    case EXCP_UNKNOWN:
    case EXCP_MSR: /* privileged instruction */
      sig_trap(ctx, SIGILL);
      break;

    case EXCP_FP_SIMD:
      thread_self()->td_pflags |= TDP_FPUINUSE;
      break;

    case EXCP_BRKPT_EL0:
    case EXCP_BRK:
      sig_trap(ctx, SIGTRAP);
      break;

    default:
      kernel_oops(ctx);
  }

  /* This is right moment to check if out time slice expired. */
  on_exc_leave();

  /* If we're about to return to user mode then check pending signals, etc. */
  on_user_exc_leave(uctx, exc_code == EXCP_SVC64 ? &result : NULL);
}

void kern_trap_handler(ctx_t *ctx) {
  register_t esr = READ_SPECIALREG(esr_el1);
  register_t far = READ_SPECIALREG(far_el1);
  register_t exc_code = ESR_ELx_EXCEPTION(esr);
  register_t dfsc = esr & ISS_DATA_DFSC_MASK;

  /* If interrupts were enabled before we trapped, then turn them on here. */
  if ((_REG(ctx, SPSR) & PSR_I) == 0)
    cpu_intr_enable();

  switch (exc_code) {
    case EXCP_INSN_ABORT:
    case EXCP_DATA_ABORT:
      klog("exc:0x%x dfsc:0x%x at %p, caused by reference to $%p!", exc_code,
           dfsc, _REG(ctx, PC), far);
      if (pmap_fault_handler(ctx, far, exc_access(exc_code, esr)))
        kernel_oops(ctx);
      break;

    default:
      kernel_oops(ctx);
  }
}

__no_profile void kern_exc_handler(ctx_t *ctx, bool intr) {
  thread_t *td = thread_self();
  assert(td->td_idnest == 0);
  assert(cpu_intr_disabled());

  ctx_t *kframe_saved = td->td_kframe;
  td->td_kframe = ctx;

  if (intr)
    intr_root_handler(ctx);
  else
    kern_trap_handler(ctx);

  td->td_kframe = kframe_saved;
}
