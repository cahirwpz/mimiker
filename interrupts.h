#ifndef __INTERRUPTS_H__
#define __INTERRUPTS_H__

#include "pic32mz.h"

/* Disables interrupts. Returns the previous value of Status
   register. */
#define di() ({                             \
    int oldval;                             \
    asm volatile ("di %0": "=r" (oldval));  \
    oldval; })

/* Re-enables interrupts previously disabled with di(). */
#define ei(val) ({mtc0(C0_STATUS,0,(val));})

/* Initializes and enables interrupts. */
void intr_init();

/* This is the single interrupt handler procedure. It should not be
 * called manually except by the interrupt handler routine from
 * intr.S.*/
void intr_dispatcher();

#endif
