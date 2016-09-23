#ifndef __MIPS_CLOCK_H__
#define __MIPS_CLOCK_H__

#include <clock.h>

/* Initializes and enables core timer interrupts. */
void mips_clock_init();

/* Processes core timer interrupts. */
void mips_clock_irq_handler();

#endif // __MIPS_CLOCK_H__
