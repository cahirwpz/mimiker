#ifndef __CLOCK_H__
#define __CLOCK_H__

/* Initializes and enables core timer interrupts. */
void clock_init();

/* Returns the number of ms passed since timer started running. */
unsigned clock_get_ms();

/* Processes core timer interrupts. */
void hardclock();

#endif // __CLOCK_H__
