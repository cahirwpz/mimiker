#define KL_LOG KL_INTR
#include <klog.h>
#include <errno.h>
#include <mips/exc.h>
#include <pmap.h>
#include <stdc.h>
#include <sysent.h>
#include <thread.h>

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

void kernel_oops(exc_frame_t *frame) {
  unsigned code = (frame->cause & CR_X_MASK) >> CR_X_SHIFT;

  klog("%s at $%08x!", exceptions[code], frame->pc);
  if ((code == EXC_ADEL || code == EXC_ADES) ||
      (code == EXC_IBE || code == EXC_DBE))
    klog("Caused by reference to $%08x!", frame->badvaddr);

  panic("Unhandled exception!");
}

void cpu_get_syscall_args(const exc_frame_t *frame, syscall_args_t *args) {
  args->code = frame->v0;
  args->args[0] = frame->a0;
  args->args[1] = frame->a1;
  args->args[2] = frame->a2;
  args->args[3] = frame->a3;
}

void syscall_handler(exc_frame_t *frame) {
  /* Eventually we will want a platform-independent syscall entry, so
     argument retrieval is done separately */
  syscall_args_t args;
  cpu_get_syscall_args(frame, &args);

  int retval = 0;

  if (args.code > SYS_LAST) {
    retval = -ENOSYS;
    goto finalize;
  }

  /* Call the handler. */
  retval = sysent[args.code].call(thread_self(), &args);

finalize:
  if (retval != -EJUSTRETURN)
    exc_frame_set_retval(frame, retval);
}

void fpe_handler(exc_frame_t *frame) {
  thread_t *td = thread_self();
  if (td->td_proc) {
    sig_send(td->td_proc, SIGFPE);
  } else {
    panic("Floating point exception or integer overflow in a kernel thread.");
  }
}

/*
 * This is exception vector table. Each exeception either has been assigned a
 * handler or kernel_oops is called for it. For exact meaning of exception
 * handlers numbers please check 5.23 Table of MIPS32 4KEc User's Manual.
 */

void *general_exception_table[32] = {[EXC_MOD] = tlb_exception_handler,
                                     [EXC_TLBL] = tlb_exception_handler,
                                     [EXC_TLBS] = tlb_exception_handler,
                                     [EXC_SYS] = syscall_handler,
                                     [EXC_FPE] = fpe_handler,
                                     [EXC_MSAFPE] = fpe_handler,
                                     [EXC_OVF] = fpe_handler};
