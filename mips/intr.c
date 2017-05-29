#define KL_LOG KL_INTR
#include <klog.h>
#include <errno.h>
#include <interrupt.h>
#include <mips/exc.h>
#include <mips/mips.h>
#include <pmap.h>
#include <pmap.h>
#include <queue.h>
#include <stdc.h>
#include <sync.h>
#include <sysent.h>
#include <thread.h>

extern const char _ebase[];

#define MIPS_INTR_CHAIN(irq, name)                                             \
  (intr_chain_t) {                                                             \
    .ic_name = (name), .ic_irq = (irq),                                        \
    .ic_handlers = TAILQ_HEAD_INITIALIZER(mips_intr_chain[irq].ic_handlers)    \
  }

static intr_chain_t mips_intr_chain[8] = {
    /* Initialize software interrupts handler chains. */
    [0] = MIPS_INTR_CHAIN(0, "swint(0)"), [1] = MIPS_INTR_CHAIN(1, "swint(1)"),
    /* Initialize hardware interrupts handler chains. */
    [2] = MIPS_INTR_CHAIN(2, "hwint(0)"), [3] = MIPS_INTR_CHAIN(3, "hwint(1)"),
    [4] = MIPS_INTR_CHAIN(4, "hwint(2)"), [5] = MIPS_INTR_CHAIN(5, "hwint(3)"),
    [6] = MIPS_INTR_CHAIN(6, "hwint(4)"), [7] = MIPS_INTR_CHAIN(7, "hwint(5)")};

void mips_intr_init() {
  /*
   * Enable Vectored Interrupt Mode as described in „MIPS32® 24KETM Processor
   * Core Family Software User’s Manual”, chapter 6.3.1.2.
   */

  /* The location of exception vectors is set to EBase. */
  mips32_set_c0(C0_EBASE, _ebase);
  mips32_bc_c0(C0_STATUS, SR_BEV);
  /* Use the special interrupt vector at EBase + 0x200. */
  mips32_bs_c0(C0_CAUSE, CR_IV);
  /* Set vector spacing to 0. */
  mips32_set_c0(C0_INTCTL, INTCTL_VS_0);
}

void mips_intr_setup(intr_handler_t *handler, unsigned irq) {
  intr_chain_t *chain = &mips_intr_chain[irq];
  CRITICAL_SECTION {
    intr_chain_add_handler(chain, handler);
    if (chain->ic_count == 1) {
      mips32_bs_c0(C0_STATUS, SR_IM0 << irq); /* enable interrupt */
      mips32_bc_c0(C0_CAUSE, CR_IP0 << irq);  /* clear pending flag */
    }
  }
}

void mips_intr_teardown(intr_handler_t *handler) {
  intr_chain_t *chain = handler->ih_chain;
  CRITICAL_SECTION {
    if (chain->ic_count == 1)
      mips32_bc_c0(C0_STATUS, SR_IM0 << chain->ic_irq);
    intr_chain_remove_handler(handler);
  }
}

void mips_intr_handler(exc_frame_t *frame) {
  unsigned pending = (frame->cause & frame->sr) & CR_IP_MASK;

  for (int i = 7; i >= 0; i--) {
    unsigned irq = CR_IP0 << i;

    if (pending & irq) {
      intr_chain_run_handlers(&mips_intr_chain[i]);
      pending &= ~irq;
    }
  }

  mips32_set_c0(C0_CAUSE, frame->cause & ~CR_IP_MASK);
}

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
