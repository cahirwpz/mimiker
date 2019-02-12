#define KL_LOG KL_INTR
#include <klog.h>
#include <errno.h>
#include <exception.h>
#include <interrupt.h>
#include <mips/exc.h>
#include <mips/intr.h>
#include <mips/mips.h>
#include <pcpu.h>
#include <pmap.h>
#include <spinlock.h>
#include <queue.h>
#include <sysent.h>
#include <sched.h>
#include <thread.h>
#include <ktest.h>

typedef void (*exc_handler_t)(exc_frame_t *);

extern const char _ebase[];

/* Extra information regarding DI / EI usage (from MIPS® ISA documentation):
 *
 * The instruction creates an execution hazard between the change to SR register
 * and the point where the change to the interrupt enable takes effect. This
 * hazard is cleared by the EHB, JALR.HB, JR.HB, or ERET instructions. Software
 * must not assume that a fixed latency will clear the execution hazard. */

void mips_intr_disable(void) {
  asm volatile("di; ehb");
}

void mips_intr_enable(void) {
  asm volatile("ei; ehb");
}

bool mips_intr_disabled(void) {
  return (mips32_getsr() & SR_IE) == 0;
}

static intr_event_t mips_intr_event[8];

#define MIPS_INTR_EVENT(irq, name)                                             \
  intr_event_init(&mips_intr_event[irq], irq, name, mips_mask_irq,             \
                  mips_unmask_irq, NULL)

static void mips_mask_irq(intr_event_t *ie) {
  int irq = ie->ie_irq;
  mips32_bc_c0(C0_STATUS, SR_IM0 << irq);
}

static void mips_unmask_irq(intr_event_t *ie) {
  int irq = ie->ie_irq;
  mips32_bs_c0(C0_STATUS, SR_IM0 << irq);
}

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

  /* Initialize software interrupts handler events. */
  MIPS_INTR_EVENT(MIPS_SWINT0, "swint(0)");
  MIPS_INTR_EVENT(MIPS_SWINT1, "swint(1)");
  /* Initialize hardware interrupts handler events. */
  MIPS_INTR_EVENT(MIPS_HWINT0, "hwint(0)");
  MIPS_INTR_EVENT(MIPS_HWINT1, "hwint(1)");
  MIPS_INTR_EVENT(MIPS_HWINT2, "hwint(2)");
  MIPS_INTR_EVENT(MIPS_HWINT3, "hwint(3)");
  MIPS_INTR_EVENT(MIPS_HWINT4, "hwint(4)");
  MIPS_INTR_EVENT(MIPS_HWINT5, "hwint(5)");

  for (unsigned i = 0; i < 8; i++)
    intr_event_register(&mips_intr_event[i]);
}

void mips_intr_setup(intr_handler_t *handler, unsigned irq) {
  intr_event_t *event = &mips_intr_event[irq];
  intr_event_add_handler(event, handler);
}

void mips_intr_teardown(intr_handler_t *handler) {
  intr_event_remove_handler(handler);
}

/* Hardware interrupt handler is called with interrupts disabled. */
static void mips_intr_handler(exc_frame_t *frame) {
  unsigned pending = (frame->cause & frame->sr) & CR_IP_MASK;

  for (int i = 7; i >= 0; i--) {
    unsigned irq = CR_IP0 << i;

    if (pending & irq) {
      intr_event_run_handlers(&mips_intr_event[i]);
      pending &= ~irq;
    }
  }
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
  if (kern_mode_p(frame))
    kernel_oops(frame);

  sig_trap(frame, SIGFPE);
}

static void cpu_handler(exc_frame_t *frame) {
  if (kern_mode_p(frame))
    kernel_oops(frame);

  int cp_id = (frame->cause & CR_CEMASK) >> CR_CESHIFT;
  if (cp_id != 1) {
    sig_trap(frame, SIGILL);
  } else {
    /* Enable FPU for interrupted context. */
    frame->sr |= SR_CU1;
  }
}

static void ri_handler(exc_frame_t *frame) {
  if (kern_mode_p(frame))
    kernel_oops(frame);

  sig_trap(frame, SIGILL);
}

/*
 * An address error exception occurs under the following circumstances:
 *  - instruction address is not aligned on a word boundary
 *  - load/store with an address is not aligned on a word/halfword boundary
 *  - reference to a kernel/supervisor address from user
 */
static void ade_handler(exc_frame_t *frame) {
  if (kern_mode_p(frame))
    kernel_oops(frame);

  sig_trap(frame, SIGBUS);
}

/*
 * This is exception vector table. Each exeception either has been assigned a
 * handler or kernel_oops is called for it. For exact meaning of exception
 * handlers numbers please check 5.23 Table of MIPS32 4KEc User's Manual.
 */

/* clang-format off */
static exc_handler_t exception_switch_table[32] = {
  [EXC_INTR] = mips_intr_handler,
  [EXC_MOD] = tlb_exception_handler,
  [EXC_TLBL] = tlb_exception_handler,
  [EXC_TLBS] = tlb_exception_handler,
  [EXC_ADEL] = ade_handler,
  [EXC_ADES] = ade_handler,
  [EXC_SYS] = syscall_handler,
  [EXC_FPE] = fpe_handler,
  [EXC_MSAFPE] = fpe_handler,
  [EXC_OVF] = fpe_handler,
  [EXC_CPU] = cpu_handler,
  [EXC_RI] = ri_handler
};
/* clang-format on */

noreturn void kernel_oops(exc_frame_t *frame) {
  unsigned code = exc_code(frame);

  kprintf("KERNEL PANIC!!! \n");
  kprintf("%s at $%08x!\n", exceptions[code], frame->pc);
  switch (code) {
    case EXC_ADEL:
    case EXC_ADES:
    case EXC_IBE:
    case EXC_DBE:
    case EXC_TLBL:
    case EXC_TLBS:
      kprintf("Caused by reference to $%08x!\n", frame->badvaddr);
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
  kprintf("HINT: Type 'info line *0x%08x' into gdb to find faulty code line.\n",
          frame->pc);
  if (ktest_test_running_flag)
    ktest_failure();
  else
    panic();
}

void kstack_overflow_handler(exc_frame_t *frame) {
  kprintf("Kernel stack overflow caught at $%08x!\n", frame->pc);
  if (ktest_test_running_flag)
    ktest_failure();
  else
    panic();
}

/* Let's consider possible contexts that caused exception/interrupt:
 *
 * 1. We came from user-space:
 *    interrupts and preemption must have been enabled
 * 2. We came from kernel-space:
 *    a. interrupts and preemption are enabled:
 *       kernel was running in regular thread context
 *    b. preemption is disabled, interrupts are enabled:
 *       kernel was running in thread context and preemption was disabled
 *       (this can happen during acquring or releasing sleep lock aka mutex)
 *    c. interrupts are disabled, preemption is implicitly disabled:
 *       kernel was running in interrupt context or acquired spin lock
 *
 * For each context we have a set of actions to be performed:
 *
 * 1. Handle interrupt with interrupts disabled or handle exception with
 *    interrupts and preemption enabled. Then check if user thread should be
 *    preempted. Finally prepare for return to user-space, for instance
 *    deliver signal to the process.
 * 2a. Same story as for (1) except we do not return to user-space.
 * 2b. Same as (2a) but do not check if the thread should be preempted.
 * 2c. Same as (2b) but do not enable interrupts for exception handling period.
 *
 * IMPORTANT! We should never call ctx_switch while interrupt is being handled
 *            or preemption is disabled.
 */
void mips_exc_handler(exc_frame_t *frame) {
  unsigned code = exc_code(frame);
  bool user_mode = user_mode_p(frame);

  assert(intr_disabled());

  exc_handler_t handler = exception_switch_table[code];

  if (!handler)
    kernel_oops(frame);

  if (!user_mode && (!(frame->sr & SR_IE) || preempt_disabled())) {
    /* We came here from kernel-space because of:
     * Case 2c: an exception, interrupts were disabled;
     * Case 2b: either interrupt or exception, preemption was disabled.
     * To prevent breaking critical section switching out is forbidden!
     *
     * In theory we can enable interrupts for the time of handling exception
     * in case 2b. In most cases handling exceptions in critical sections
     * will end up in kernel panic, since such scenario is usually caused
     * by programmer's error.
     *
     * XXX Being a very peculiar scenario we leave it as is for later
     *     consideration. Disabled interrupt will ease debugging.
     */
    PCPU_SET(no_switch, true);
    (*handler)(frame);
    PCPU_SET(no_switch, false);
  } else {
    /* Case 1 & 2a: we came from user-space or kernel-space,
     * interrupts and preemption were enabled! */
    assert(frame->sr & SR_IE);
    assert(!preempt_disabled());

    if (code != EXC_INTR) {
      /* We assume it is safe to handle an exception with interrupts enabled. */
      intr_enable();
      (*handler)(frame);
    } else {
      /* Switching out while handling interrupt is forbidden! */
      PCPU_SET(no_switch, true);
      (*handler)(frame);
      PCPU_SET(no_switch, false);
      /* We did the job, so we don't need interrupts to be disabled anymore. */
      intr_enable();
    }

    /* This is right moment to check if out time slice expired. */
    on_exc_leave();

    /* If we're about to return to user mode then check pending signals, etc. */
    if (user_mode)
      on_user_exc_leave();

    /* Disable interrupts for the time interrupted context is being restored. */
    intr_disable();
  }

  assert(intr_disabled());
}
