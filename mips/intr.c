#define KL_LOG KL_INTR
#include <klog.h>
#include <errno.h>
#include <exception.h>
#include <interrupt.h>
#include <mips/exc.h>
#include <mips/intr.h>
#include <mips/mips.h>
#include <pmap.h>
#include <spinlock.h>
#include <queue.h>
#include <sysent.h>
#include <thread.h>

typedef void (*exc_handler_t)(exc_frame_t *);

extern const char _ebase[];

void mips_intr_disable(void) {
  asm volatile("di");
}

void mips_intr_enable(void) {
  asm volatile("ei");
}

bool mips_intr_disabled(void) {
  return (mips32_getsr() & SR_IE) == 0;
}

#define MIPS_INTR_CHAIN(irq, name)                                             \
  [irq] = (intr_chain_t) {                                                     \
    .ic_name = (name), .ic_irq = (irq),                                        \
    .ic_handlers = TAILQ_HEAD_INITIALIZER(mips_intr_chain[irq].ic_handlers)    \
  }

static intr_chain_t mips_intr_chain[8] = {
  /* Initialize software interrupts handler chains. */
  MIPS_INTR_CHAIN(MIPS_SWINT0, "swint(0)"),
  MIPS_INTR_CHAIN(MIPS_SWINT1, "swint(1)"),
  /* Initialize hardware interrupts handler chains. */
  MIPS_INTR_CHAIN(MIPS_HWINT0, "hwint(0)"),
  MIPS_INTR_CHAIN(MIPS_HWINT1, "hwint(1)"),
  MIPS_INTR_CHAIN(MIPS_HWINT2, "hwint(2)"),
  MIPS_INTR_CHAIN(MIPS_HWINT3, "hwint(3)"),
  MIPS_INTR_CHAIN(MIPS_HWINT4, "hwint(4)"),
  MIPS_INTR_CHAIN(MIPS_HWINT5, "hwint(5)")};

void mips_intr_init(void) {
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

  for (unsigned i = 0; i < 8; i++)
    intr_chain_register(&mips_intr_chain[i]);
}

void mips_intr_setup(intr_handler_t *handler, unsigned irq) {
  intr_chain_t *chain = &mips_intr_chain[irq];
  WITH_SPINLOCK(&chain->ic_lock) {
    intr_chain_add_handler(chain, handler);
    if (chain->ic_count == 1) {
      mips32_bs_c0(C0_STATUS, SR_IM0 << irq); /* enable interrupt */
      mips32_bc_c0(C0_CAUSE, CR_IP0 << irq);  /* clear pending flag */
    }
  }
}

void mips_intr_teardown(intr_handler_t *handler) {
  intr_chain_t *chain = handler->ih_chain;
  WITH_SPINLOCK(&chain->ic_lock) {
    if (chain->ic_count == 1)
      mips32_bc_c0(C0_STATUS, SR_IM0 << chain->ic_irq);
    intr_chain_remove_handler(handler);
  }
}

/* Hardware interrupt handler is called with interrupts disabled. */
void mips_intr_handler(exc_frame_t *frame) {
  assert(intr_disabled());

  unsigned pending = (frame->cause & frame->sr) & CR_IP_MASK;

  for (int i = 7; i >= 0; i--) {
    unsigned irq = CR_IP0 << i;

    if (pending & irq) {
      intr_chain_run_handlers(&mips_intr_chain[i]);
      pending &= ~irq;
    }
  }

  mips32_set_c0(C0_CAUSE, frame->cause & ~CR_IP_MASK);

  exc_before_leave(frame);

  assert(intr_disabled());
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

static void cpu_get_syscall_args(const exc_frame_t *frame,
                                 syscall_args_t *args) {
  args->code = frame->v0;
  args->args[0] = frame->a0;
  args->args[1] = frame->a1;
  args->args[2] = frame->a2;
  args->args[3] = frame->a3;
}

static void syscall_handler(exc_frame_t *frame) {
  assert(!intr_disabled());

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

static void fpe_handler(exc_frame_t *frame) {
  thread_t *td = thread_self();
  if (td->td_proc) {
    sig_send(td->td_proc, SIGFPE);
  } else {
    panic("Floating point exception or integer overflow in a kernel thread.");
  }
}

static void cp_unusable_handler(exc_frame_t *frame) {
  int cp_id = (frame->cause & CR_CEMASK) >> CR_CESHIFT;
  bool kernel_mode = in_kernel_mode(frame);

  if (cp_id != 1) {
    panic(
      "Unexpected unusable coprocessor exception, with coprocessor id = %d\n",
      cp_id);
  }

  if (kernel_mode) {
    panic("FPU unusable exception in kernel mode.");
  }

  /* Enable FPU for interrupted context. */
  frame->sr |= SR_CU1;
}

/*
 * This is exception vector table. Each exeception either has been assigned a
 * handler or kernel_oops is called for it. For exact meaning of exception
 * handlers numbers please check 5.23 Table of MIPS32 4KEc User's Manual.
 */

static exc_handler_t user_exception_table[32] =
  {[EXC_MOD] = tlb_exception_handler,
   [EXC_TLBL] = tlb_exception_handler,
   [EXC_TLBS] = tlb_exception_handler,
   [EXC_SYS] = syscall_handler,
   [EXC_FPE] = fpe_handler,
   [EXC_MSAFPE] = fpe_handler,
   [EXC_OVF] = fpe_handler,
   [EXC_CPU] = cp_unusable_handler};

static exc_handler_t kernel_exception_table[32] =
  {[EXC_MOD] = tlb_exception_handler, [EXC_TLBL] = tlb_exception_handler,
   [EXC_TLBS] = tlb_exception_handler};

static inline unsigned exc_code(exc_frame_t *frame) {
  return (frame->cause & CR_X_MASK) >> CR_X_SHIFT;
}

noreturn void kernel_oops(exc_frame_t *frame) {
  unsigned code = exc_code(frame);

  klog("%s at $%08x!", exceptions[code], frame->pc);
  if ((code == EXC_ADEL || code == EXC_ADES) ||
      (code == EXC_IBE || code == EXC_DBE) ||
      (code == EXC_TLBL || code == EXC_TLBS))
    klog("Caused by reference to $%08x!", frame->badvaddr);

  panic("Unhandled '%s' at $%08x!", exceptions[code], frame->pc);
}

/* General exception handler is called with interrupts disabled. */
void mips_exc_handler(exc_frame_t *frame) {
  unsigned code = exc_code(frame);
  bool kernel_mode = in_kernel_mode(frame);

  assert(intr_disabled());

  if (code == EXC_INTR && kernel_mode) {
    mips_intr_handler(frame);
    return;
  }

  exc_handler_t handler =
    (kernel_mode ? kernel_exception_table : user_exception_table)[code];

  if (!handler)
    kernel_oops(frame);

  /* TODO If `handler` is not hardware interrupt handler, then it should be
   * called with interrupts enabled. Preemption state should not be altered. */
  if (code == EXC_SYS) {
    /* Handle system calls with interrupts enabled! */
    intr_enable();
    (*handler)(frame);
    intr_disable();
  } else {
    (*handler)(frame);
  }

  exc_before_leave(frame);

  assert(intr_disabled());
}
