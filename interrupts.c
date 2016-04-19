#include <interrupts.h>
#include <uart_cbus.h>
#include <clock.h>
#include <libkern.h>

extern const char _ebase[];

void intr_init() {
  /* Use the special interrupt vector EBase + 0x200. */
  mips32_bs_c0(C0_CAUSE, CR_IV);
  mips32_bc_c0(C0_STATUS, SR_BEV);
  mips32_set_c0(C0_EBASE, _ebase);
  mips32_set_c0(C0_INTCTL, 0);

  mips32_set_c0(C0_STATUS, mips32_get_c0(C0_STATUS) & ~SR_IPL_MASK);

  /* Enable interrupts. */
  mips32_bs_c0(C0_STATUS, SR_IE);
}

void intr_dispatcher() {
  unsigned irq_n = mips32_get_c0(C0_CAUSE) & CR_IP_MASK;
  /* Recognize interrupt type. */
  if (irq_n & CR_IP7) {
    /* Core timer interrupt. */
    hardclock();
  } else {
    kprintf("Received unrecognized interrupt: %d!\n", irq_n);
  }
}
