#ifndef __INTERRUPTS_H__
#define __INTERRUPTS_H__

#include "common.h"

/* Disables interrupts. Returns the previous value of Status
   register. */
#define di() _mips_intdisable()

/* Re-enables interrupts previously disabled with di(). */
#define ei(val) mips32_set_c0(C0_STATUS, (val))

/* Initializes and enables interrupts. */
void intr_init();

/* This is the single interrupt handler procedure. It should not be
 * called manually except by the interrupt handler routine from
 * intr.S.*/
void intr_dispatcher();

#endif
