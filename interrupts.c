#include "interrupts.h"
#include <libkern.h>

/* Provided by linker script. */
extern const char __ebase[];

/* This counter is incremented every millisecond. */
static volatile unsigned int timer_ms_count;

void init_interrupts(){

    /* Changing EBase must be done with Status:BEV set to 1. */
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


void init_timer(){
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

unsigned timer_get_ms(){
    return timer_ms_count;
}

void interrupt_handler(){
    unsigned irq_n = PIC32_INTSTAT_VEC(INTSTAT);
    unsigned compare,count;
    /* Recognize interrupt type. */
    switch(irq_n){
    case PIC32_IRQ_CT:
        /* Core timer interrupt. */

        /* Mark the interrupt as handled. */
        //IFSCLR(0) = 1 << PIC32_IRQ_CT;

        compare = mfc0(C0_COMPARE, 0);
        count   = mfc0(C0_COUNT, 0);
        signed int diff = compare - count;
        if (diff > 0){
            /* Should not happen. Potentially spurious interrupt. */
            return;
        }

        /* This loop is necessary, because sometimes we may miss some ticks. */
        while (diff < TICKS_PER_MS){

            compare += TICKS_PER_MS;

            /* Increment the ms counter. This increment is atomic, because
               entire _interrupt_handler disables nested interrupts. */
            timer_ms_count += 1;

            diff = compare - count;
        }
        /* Set compare register. */
        mtc0(C0_COMPARE, 0, compare);

        break;
    default:
        kprintf("Received unrecognized interrupt: %d!\n", irq_n);
    }
}
