#include "clock.h"
#include "interrupts.h"
#include "global_config.h"

/* This counter is incremented every millisecond. */
static volatile unsigned int timer_ms_count;

void clock_init()
{
    /* Disable interrupts while we are configuring them. */
    unsigned s = di();

    /* Clear instruction counter. */
    mtc0 (C0_COUNT, 0, 0);
    /* Set compare register. */
    mtc0 (C0_COMPARE, 0, TICKS_PER_MS);

    /* Clear ms counter. */
    timer_ms_count = 0;

    /* Clear core timer interrupt status. */
    IFSSET(0) = 1 << 0;
    /* Set core timer interrupts priority to 6 (highest) */
    unsigned p = IPC(0);
    p &= ~PIC32_IPC_IP0(7); // Clear priority 0 bits
    p |=  PIC32_IPC_IP0(6); // Set them to 6
    IPC(0) = p;
    /* Enable core timer interrupts. */
    IECSET(0) = 1 << 0;

    /* It is safe now to re-enable interrupts. */
    ei(s);
}

unsigned clock_get_ms()
{
    return timer_ms_count;
}


void hardclock()
{
    unsigned compare = mfc0(C0_COMPARE, 0);
    unsigned count   = mfc0(C0_COUNT, 0);
    signed int diff = compare - count;
    if (diff > 0) {
        /* Should not happen. Potentially spurious interrupt. */
        return;
    }

    /* This loop is necessary, because sometimes we may miss some ticks. */
    while (diff < TICKS_PER_MS) {

        compare += TICKS_PER_MS;

        /* Increment the ms counter. This increment is atomic, because
           entire _interrupt_handler disables nested interrupts. */
        timer_ms_count += 1;

        diff = compare - count;
    }
    /* Set compare register. */
    mtc0(C0_COMPARE, 0, compare);
}
