/*
 * Include processor definitions.
 */
#include "pic32mz.h"
#include "uart_raw.h"
#include "global_config.h"
#include "interrupts.h"
#include "clock.h"

#include <libkern.h>

char str[] = "This is a global string!\n";
char empty[100]; /* This should land in .bss and get cleared by _start procedure. */

/*
 * Chip configuration.
 */
PIC32_DEVCFG (
    DEVCFG0_JTAG_DISABLE |      /* Disable JTAG port */
    DEVCFG0_TRC_DISABLE,        /* Disable trace port */

    /* Using primary oscillator with external crystal 24 MHz.
     * PLL multiplies it to 200 MHz. */
    DEVCFG1_FNOSC_SPLL |        /* System clock supplied by SPLL */
    DEVCFG1_POSCMOD_EXT |       /* External generator */
    DEVCFG1_FCKS_ENABLE |       /* Enable clock switching */
    DEVCFG1_FCKM_ENABLE |       /* Enable fail-safe clock monitoring */
    DEVCFG1_IESO |              /* Internal-external switch over enable */
    DEVCFG1_CLKO_DISABLE,       /* CLKO output disable */

    DEVCFG2_FPLLIDIV_3 |        /* PLL input divider = 3 */
    DEVCFG2_FPLLRNG_5_10 |      /* PLL input range is 5-10 MHz */
    DEVCFG2_FPLLMULT(50) |      /* PLL multiplier = 50x */
    DEVCFG2_FPLLODIV_2,         /* PLL postscaler = 1/2 */

    DEVCFG3_FETHIO |            /* Default Ethernet pins */
    DEVCFG3_USERID(0xffff));    /* User-defined ID */

static volatile unsigned loop;

/*
 * Delay for a given number of microseconds.
 * The processor has a 32-bit hardware Count register,
 * which increments at half CPU rate.
 * We use it to get a precise delay.
 */
void udelay (unsigned usec)
{
    unsigned now = mfc0 (C0_COUNT, 0);
    unsigned final = now + usec * MHZ / 2;

    for (;;) {
        now = mfc0 (C0_COUNT, 0);

        /* This comparison is valid only when using a signed type. */
        if ((int) (now - final) >= 0)
            break;
    }
}

/*
 * Delays for at least the given number of milliseconds.  May not be
 * nanosecond-accurate.
 */
void mdelay (unsigned msec)
{
    unsigned now = clock_get_ms();
    unsigned final = now + msec;
    while(final > clock_get_ms());
}

int kernel_main()
{
    /* Initialize coprocessor 0. */
    //mtc0 (C0_CAUSE, 0, 1 << 23);        /* Set IV */
    //mtc0 (C0_STATUS, 0, 0);             /* Clear BEV */

    /* Use pins PA0-PA3, PF13, PF12, PA6-PA7 as output: LED control. */
    LATACLR = 0xCF;
    TRISACLR = 0xCF;
    LATFCLR = 0x3000;
    TRISFCLR = 0x3000;

    intr_init();

    /* Initialize UART. */
    uart_init();
    kprintf ("Hello, UART!\n");

    /* Demonstrate access to .data */
    kprintf ("%s",str);

    /* Test whether .bss appears to have been cleared. */
    char* p = empty;
    while(p < empty + sizeof(empty))
        if(*p++ != 0x00)
            kprintf("Apparently .bss was not cleared!\n");
            // TODO: Exit main? Ignore?

    /*
     * Print initial state of control registers.
     */
    kprintf ("-\n");
    kprintf ("Status   = 0x%08x\n", mfc0(12, 0));
    kprintf ("IntCtl   = 0x%08x\n", mfc0(12, 1));
    kprintf ("SRSCtl   = 0x%08x\n", mfc0(12, 2));
    kprintf ("Cause    = 0x%08x\n", mfc0(13, 0));
    kprintf ("PRId     = 0x%08x\n", mfc0(15, 0));
    kprintf ("EBase    = 0x%08x\n", mfc0(15, 1));
    kprintf ("CDMMBase = 0x%08x\n", mfc0(15, 2));
    kprintf ("Config   = 0x%08x\n", mfc0(16, 0));
    kprintf ("Config1  = 0x%08x\n", mfc0(16, 1));
    kprintf ("Config2  = 0x%08x\n", mfc0(16, 2));
    kprintf ("Config3  = 0x%08x\n", mfc0(16, 3));
    kprintf ("Config4  = 0x%08x\n", mfc0(16, 4));
    kprintf ("Config5  = 0x%08x\n", mfc0(16, 5));
    kprintf ("Config7  = 0x%08x\n", mfc0(16, 7));
    kprintf ("WatchHi  = 0x%08x\n", mfc0(19, 0));
    kprintf ("WatchHi1 = 0x%08x\n", mfc0(19, 1));
    kprintf ("WatchHi2 = 0x%08x\n", mfc0(19, 2));
    kprintf ("WatchHi3 = 0x%08x\n", mfc0(19, 3));
    kprintf ("Debug    = 0x%08x\n", mfc0(23, 0));
    kprintf ("PerfCtl0 = 0x%08x\n", mfc0(25, 0));
    kprintf ("PerfCtl1 = 0x%08x\n", mfc0(25, 2));
    kprintf ("DEVID    = 0x%08x\n", DEVID      );
    kprintf ("OSCCON   = 0x%08x\n", OSCCON     );
    kprintf ("DEVCFG0  = 0x%08x\n", DEVCFG0    );
    kprintf ("DEVCFG1  = 0x%08x\n", DEVCFG1    );
    kprintf ("DEVCFG2  = 0x%08x\n", DEVCFG2    );
    kprintf ("DEVCFG3  = 0x%08x\n", DEVCFG3    );

    clock_init();

    unsigned last = 0;
    while (1) {
        /* Invert pins PA7-PA0. */
        LATAINV = 1 << 0;  mdelay (100);
        LATAINV = 1 << 1;  mdelay (100);
        LATAINV = 1 << 2;  mdelay (100);
        LATAINV = 1 << 3;  mdelay (100);
        LATFINV = 1 << 13; mdelay (100);
        LATFINV = 1 << 12; mdelay (100);
        LATAINV = 1 << 6;  mdelay (100);
        LATAINV = 1 << 7;  mdelay (100);

        mdelay(200);

        loop++;
        unsigned curr = clock_get_ms();
        kprintf("Milliseconds since timer start: %d (diff: %d)\n", curr, curr - last);
        last = curr;
    }
}
