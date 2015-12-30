#ifndef __INTERRUPTS_H__
#define __INTERRUPTS_H__

#include "pic32mz.h"
#include "global_config.h"

/* Disables interrupts. Returns the previous value of Status
   register. */
#define di() ({                             \
    int oldval;                             \
    asm volatile ("di %0": "=r" (oldval));  \
    oldval; })

/* Re-enables interrupts previously disabled with di(). */
#define ei(val) ({mtc0(C0_STATUS,0,(val));})

/* Initializes and enables interrupts. */
void init_interrupts();

/* This is the single interrupt handler procedure. */
void interrupt_handler();


/* -- CORE TIMER -- */

/* Initializes and enables core timer interrupts. */
void init_timer();

/* Returns the number of ms passed since timer started running. */
unsigned timer_get_ms();


#endif
