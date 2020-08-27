#include <sys/libkern.h>
#include <sys/exception.h>
#include <mips/mips.h>
#include <mips/exception.h>
#include <mips/pmap.h>

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

void exc_frame_init(exc_frame_t *frame, void *pc, void *sp, unsigned flags) {
  bool usermode = flags & EF_USER;

  bzero(frame, usermode ? sizeof(exc_frame_t) : sizeof(cpu_exc_frame_t));

  frame->gp = usermode ? 0 : (register_t)_gp;
  frame->pc = (register_t)pc;
  frame->sp = (register_t)sp;

  /* For user-mode exception frame we must make sure that:
   * - user mode is active,
   * - interrupts are enabled.
   * The rest will be covered by usr_exc_leave. */
  frame->sr = mips32_get_c0(C0_STATUS) | SR_IE | (usermode ? SR_KSU_USER : 0);
}

void exc_frame_copy(exc_frame_t *to, exc_frame_t *from) {
  memcpy(to, from, sizeof(exc_frame_t));
}

void exc_frame_setup_call(exc_frame_t *frame, register_t retaddr,
                          register_t arg) {
  frame->ra = retaddr;
  frame->a0 = arg;
}

void exc_frame_set_retval(exc_frame_t *frame, register_t value,
                          register_t error) {
  frame->v0 = (register_t)value;
  frame->v1 = (register_t)error;
  frame->pc += 4;
}
