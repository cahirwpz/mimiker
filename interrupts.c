#include "interrupts.h"
#include "clock.h"
#include "pic32mz.h"
#include <libkern.h>

/* Provided by linker script. */
extern const char __ebase[];

void intr_init() {
  /*
   * PIC32 provides various interrupt modes it can be configured to use.
   * Because of QEMU limits, the configuration we use is to enable External
   * Interrupts Controller, and configure it to use vector spacing 0 (so there
   * is a single interrupt handler).
   */

  /* 
   * The C0's EBase register stores the base address of interrupt handlers. In
   * case of vectored interrupts, the actual handler address is calculated by
   * the hadrware using formula:
   *
   *    EBase + vector_spacing * vector_no + 0x200.
   *
   * This way EBase might be used to quickly, globally switch to a different
   * set of handlers.  Since we use vector_spacing = 0, the formula simplifies
   * significantly.
   */

  /*
   * EIC interrupt mode is in effect if all of the following conditions are
   * true:
   * - Config3:VEIC = 1
   * - IntCtl:VS != 0
   * - Cause:IV = 1
   * - Status:BEV = 0
   */

  /*
   * For no clear reason, the architecture requires that changing EBase has to
   * be done with Status:BEV set to 1. We will restore it to 0 afterwards,
   * because BEV=1 enables Legacy (non-vectored) Interrupt Mode.
   */
  mips32_bs_c0(C0_STATUS, SR_BEV);
  mips32_set_c0(C0_EBASE, __ebase);
  mips32_bc_c0(C0_STATUS, SR_BEV);

  /*
   * Set internal interrupt vector spacing to 32. This value will not be used,
   * because it is the External Interrupt Controller that will calculate
   * handler adresses. However, this value must be non-zero in order to enable
   * vectored interrupts.
   */
  mips32_set_c0(C0_INTCTL, 32);

  mips32_bs_c0(C0_CONFIG3, CFG3_VEIC);
  mips32_bs_c0(C0_CAUSE, CR_IV);

  /* Set EIC's vector spacing to 0. */
  INTCON = 0;

  /* Clear interrupt status. */
  IFS(0) = 0;
  IFS(1) = 0;
  IFS(2) = 0;
  IFS(3) = 0;
  IFS(4) = 0;
  IFS(5) = 0;

  /* Enable interrupts. */
  mips32_bs_c0(C0_STATUS, SR_IE);
}

void intr_dispatcher() {
  unsigned irq_n = PIC32_INTSTAT_VEC(INTSTAT);
  /* Recognize interrupt type. */
  switch (irq_n) {
    case PIC32_IRQ_CT:
      /* Core timer interrupt. */
      hardclock();
      IFSCLR(0) = 1 << PIC32_IRQ_CT;
      break;
    default:
      kprintf("Received unrecognized interrupt: %d!\n", irq_n);
  }
}
