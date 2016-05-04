#include <interrupts.h>
#include <libkern.h>
#include <mips.h>

extern const char _ebase[];

void intr_init() {
  /*
   * Enable Vectored Interrupt Mode as described in „MIPS32® 24KETM Processor
   * Core Family Software User’s Manual”, chapter 6.3.1.2.
   */

  /* The location of exception vectors is set to EBase. */
  mips32_set_c0(C0_EBASE, _ebase);
  mips32_bc_c0(C0_STATUS, SR_BEV);
  /* Use the special interrupt vector at EBase + 0x200. */
  mips32_bs_c0(C0_CAUSE, CR_IV);
  /* Set vector spacing for 0x20. */
  mips32_set_c0(C0_INTCTL, INTCTL_VS_32);

  /*
   * Mask out software and hardware interrupts. 
   * You should enable them one by one in driver initialization code.
   */
  mips32_set_c0(C0_STATUS, mips32_get_c0(C0_STATUS) & ~SR_IPL_MASK);

  intr_enable();
}

static const char *exceptions[32] = {
  [EXC_INTR] = "Interrupt",
  [EXC_MOD]  = "TLB modification exception",
  [EXC_TLBL] = "TLB exception (load or instruction fetch)",
  [EXC_TLBS] = "TLB exception (store)",
  [EXC_ADEL] = "Address error exception (load or instruction fetch)",
  [EXC_ADES] = "Address error exception (store)",
  [EXC_IBE]  = "Bus error exception (instruction fetch)",
  [EXC_DBE]  = "Bus error exception (data reference: load or store)",
  [EXC_BP]   = "Breakpoint exception",
  [EXC_RI]   = "Reserved instruction exception",
  [EXC_CPU]  = "Coprocessor Unusable exception",
  [EXC_OVF]  = "Arithmetic Overflow exception",
  [EXC_TRAP] = "Trap exception",
  [EXC_FPE]  = "Floating point exception",
  [EXC_WATCH] = "Reference to watchpoint address",
  [EXC_MCHECK] = "Machine checkcore",
};

void kernel_oops() {
  int code = (mips32_get_c0(C0_CAUSE) & CR_X_MASK) >> CR_X_SHIFT;

  kprintf("[oops] %s at $%08x!\n", exceptions[code], mips32_get_c0(C0_ERRPC));

  if (code == EXC_ADEL || code == EXC_ADES)
    kprintf("[oops] Caused by reference to $%08x!\n", mips32_get_c0(C0_BADVADDR));
}
