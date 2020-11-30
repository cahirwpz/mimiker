#include <sys/klog.h>
#include <sys/context.h>
#include <sys/mimiker.h>
#include <sys/thread.h>
#include <sys/pmap.h>
#include <sys/vm_physmem.h>
#include <sys/vm_map.h>
#include <sys/syscall.h>
#include <sys/sysent.h>
#include <sys/errno.h>
#include <aarch64/armreg.h>
#include <aarch64/context.h>
#include <aarch64/interrupt.h>
#include <aarch64/pmap.h>
#include <aarch64/fpu.h>

static __noreturn void kernel_oops(ctx_t *ctx) {
  kprintf("KERNEL PANIC!!! \n");
  panic();
}

static void syscall_handler(register_t code, ctx_t *ctx) {
  register_t args[SYS_MAXSYSARGS];
  const int nregs = 8;

  memcpy(args, &_REG(ctx, X0), nregs * sizeof(register_t));

  if (code > SYS_MAXSYSCALL) {
    args[0] = code;
    code = 0;
  }

  sysent_t *se = &sysent[code];
  size_t nargs = se->nargs;

  assert(nargs <= nregs);

  thread_t *td = thread_self();
  register_t retval = 0;

  assert(td->td_proc != NULL);

  int error = se->call(td->td_proc, (void *)args, &retval);

  if (error != EJUSTRETURN)
    user_ctx_set_retval((user_ctx_t *)ctx, error ? -1 : retval, error);
}

static void abort_handler(ctx_t *ctx, register_t esr, vaddr_t vaddr,
                          bool usermode) {
  uint32_t exception = ESR_ELx_EXCEPTION(esr);
  thread_t *td = thread_self();

  klog("%x at $%lx, caused by reference to $%lx!", exception, _REG(ctx, PC),
       vaddr);

  pmap_t *pmap = pmap_lookup(vaddr);
  if (!pmap) {
    klog("No physical map defined for %lx address!", vaddr);
    goto fault;
  }

  vm_prot_t access = VM_PROT_READ;

  if (exception == EXCP_INSN_ABORT || exception == EXCP_INSN_ABORT_L) {
    access = VM_PROT_EXEC;
  } else if (esr & ISS_DATA_WnR) {
    access |= VM_PROT_WRITE;
  }

  paddr_t pa;
  if (pmap_extract(pmap, vaddr, &pa)) {
    vm_page_t *pg = vm_page_find(pa);

    if (access & (VM_PROT_READ | VM_PROT_EXEC)) {
      pmap_set_referenced(pg);
    } else if (access & VM_PROT_WRITE) {
      pmap_set_referenced(pg);
      pmap_set_modified(pg);
    } else {
      kernel_oops(ctx);
    }

    return;
  }

  vm_map_t *vmap = vm_map_lookup(vaddr);
  if (!vmap) {
    klog("No virtual address space defined for %lx!", vaddr);
    goto fault;
  }
  if (vm_page_fault(vmap, vaddr, access) == 0)
    return;

fault:
  if (td->td_onfault) {
    _REG(ctx, PC) = td->td_onfault;
    td->td_onfault = 0;
  } else if (usermode) {
    /* Send a segmentation fault signal to the user program. */
    sig_trap(ctx, SIGSEGV);
  } else {
    kernel_oops(ctx);
  }
}

void user_trap_handler(user_ctx_t *uctx) {
  /* Let's read special registers before enabling interrupts.
   * This ensures their values will not be lost. */
  ctx_t *ctx = (ctx_t *)uctx;
  register_t esr = READ_SPECIALREG(esr_el1);
  register_t far = READ_SPECIALREG(far_el1);
  register_t exception = ESR_ELx_EXCEPTION(esr);

  if (fpu_enabled())
    fpu_save_ctx(uctx);

  cpu_intr_enable();

  assert(!intr_disabled() && !preempt_disabled());

  switch (exception) {
    case EXCP_INSN_ABORT_L:
    case EXCP_INSN_ABORT:
    case EXCP_DATA_ABORT_L:
    case EXCP_DATA_ABORT:
      abort_handler(ctx, esr, far, true);
      break;

    case EXCP_SVC64:
      syscall_handler(ESR_ELx_SYSCALL(esr), ctx);
      break;

    case EXCP_SP_ALIGN:
    case EXCP_PC_ALIGN:
      sig_trap(ctx, SIGBUS);
      break;

    case EXCP_MSR: /* privileged instruction */
      sig_trap(ctx, SIGILL);
      break;

    case EXCP_FP_SIMD:
      fpu_enable();
      break;

    default:
      kernel_oops(ctx);
  }

  /* This is right moment to check if out time slice expired. */
  on_exc_leave();

  /* If we're about to return to user mode then check pending signals, etc. */
  on_user_exc_leave();

  if (exception != EXCP_FP_SIMD && fpu_enabled())
    fpu_load_ctx(uctx);
}

void kern_trap_handler(ctx_t *ctx) {
  register_t esr = READ_SPECIALREG(esr_el1);
  register_t far = READ_SPECIALREG(far_el1);

  /* If interrupts were enabled before we trapped, then turn them on here. */
  if ((_REG(ctx, SPSR) & DAIF_I_MASKED) == 0)
    cpu_intr_enable();

  switch (ESR_ELx_EXCEPTION(esr)) {
    case EXCP_INSN_ABORT:
    case EXCP_DATA_ABORT:
      abort_handler(ctx, esr, far, false);
      break;

    default:
      kernel_oops(ctx);
  }
}
