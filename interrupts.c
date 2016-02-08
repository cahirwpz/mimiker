#include "interrupts.h"
#include "clock.h"
#include <libkern.h>

/* Provided by linker script. */
extern const char __ebase[];

void intr_init() {

  /* PIC32 provides various interrupt modes it can be configured to
   * use. Because of QEMU limits, the configuration we use is to
   * enable External Interrupts Controller, and configure it to use
   * vector spacing 0 (so there is a single interrupt handler).
   */

  /* The C0's EBase register stores the base address of interrupt
   * handlers. In case of vectored interrupts, the actual handler
   * address is calculated by the hadrware using formula:
   *
   *    EBase + vector_spacing * vector_no + 0x200.
   *
   * This way EBase might be used to quickly, globally switch to a
   * different set of handlers.  Since we use vector_spacing = 0,
   * the formula simplifies significantly.
   */

  /* For no clear reason, the architecture requires that changing
   * EBase has to be done with Status:BEV set to 1. We will restore
   * it to 0 afterwards, because BEV=1 enables Legacy (non-vectored)
   * Interrupt Mode.
   */
  unsigned status = mfc0(C0_STATUS, 0);
  mtc0(C0_STATUS, 0, status | ST_BEV);

  /* Set EBase. */
  mtc0(C0_EBASE, 1, __ebase);

  /* Restore Status, set BEV to 0. */
  status &= ~ST_BEV;
  mtc0(C0_STATUS, 0, status);

  /* Set internal interrupt vector spacing to 32. This value will
     not be used, because it is the External Interrupt Controller
     that will calculate handler adresses. However, this value must
     be non-zero in order to enable vectored interrupts. */
  mtc0(C0_INTCTL, 1, 1 << 5);

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
  status |= ST_IE;
  mtc0(C0_STATUS, 0, status);
}

void intr_dispatcher() {
  unsigned irq_n = PIC32_INTSTAT_VEC(INTSTAT);
  /* Recognize interrupt type. */
  switch (irq_n) {
    case PIC32_IRQ_CT:
      /* Core timer interrupt. */
      hardclock();
      break;
    default:
      kprintf("Received unrecognized interrupt: %d!\n", irq_n);
  }
}
