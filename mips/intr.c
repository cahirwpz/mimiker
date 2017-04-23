#include <stdc.h>
#include <mips/exc.h>
#include <mips/mips.h>
#include <pmap.h>
#include <sysent.h>
#include <errno.h>
#include <thread.h>

extern const char _ebase[];

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

extern void mips_clock_irq_handler();

typedef void (*irq_handler_t)();

static irq_handler_t irq_handlers[8] = {[7] = mips_clock_irq_handler};

void mips_irq_handler(exc_frame_t *frame) {
  unsigned pending = (frame->cause & frame->sr) & CR_IP_MASK;

  for (int i = 7; i >= 0; i--) {
    unsigned irq = CR_IP0 << i;

    if (pending & irq) {
      irq_handler_t handler = irq_handlers[i];
      if (handler != NULL) {
        handler();
      } else {
        log("Spurious hardware interrupt #%d!", i);
      }
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

  log("%s at $%08x!", exceptions[code], frame->pc);
  if ((code == EXC_ADEL || code == EXC_ADES) ||
      (code == EXC_IBE || code == EXC_DBE))
    log("Caused by reference to $%08x!", frame->badvaddr);

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

/*
 * This is exception vector table. Each exeception either has been assigned a
 * handler or kernel_oops is called for it. For exact meaning of exception
 * handlers numbers please check 5.23 Table of MIPS32 4KEc User's Manual.
 */

void *general_exception_table[32] = {
    [EXC_MOD] = tlb_exception_handler, [EXC_TLBL] = tlb_exception_handler,
    [EXC_TLBS] = tlb_exception_handler, [EXC_SYS] = syscall_handler,
};
