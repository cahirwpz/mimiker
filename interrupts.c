#include <interrupts.h>
#include <uart_cbus.h>
#include <clock.h>
#include <libkern.h>

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
