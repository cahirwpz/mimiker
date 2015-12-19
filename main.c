/*
 * Include processor definitions.
 */
#include "pic32mz.h"
#include "uart_raw.h"
#include "global_config.h"

#include <stdio.h>
#include <string.h>
#include <limits.h>

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

void stdlib_demo();

int kernel_main()
{
    /* Initialize coprocessor 0. */
    mtc0 (C0_COUNT, 0, 0);
    mtc0 (C0_COMPARE, 0, -1);
    //mtc0 (C0_EBASE, 1, 0x9fc00000);     /* Vector base */
    //mtc0 (C0_INTCTL, 1, 1 << 5);        /* Vector spacing 32 bytes */
    //mtc0 (C0_CAUSE, 0, 1 << 23);        /* Set IV */
    //mtc0 (C0_STATUS, 0, 0);             /* Clear BEV */

    /* Use pins PA0-PA3, PF13, PF12, PA6-PA7 as output: LED control. */
    LATACLR = 0xCF;
    TRISACLR = 0xCF;
    LATFCLR = 0x3000;
    TRISFCLR = 0x3000;

    /* Initialize UART. */
    uart_init();
    uart_puts("Hello, UART!\n");

    /* Demonstrate access to .data */
    uart_puts(str);

    /* Test whether .bss appears to have been cleared. */
    char* p = empty;
    while(p < empty + sizeof(empty))
        if(*p++ != 0x00)
            uart_puts("Apparently .bss was not cleared!\n");
            // TODO: Exit main? Ignore?

    /*
     * Print initial state of control registers.
     */
    uart_putc ('-');
    uart_putc ('\n');
    uart_printreg ("Status  ", mfc0(12, 0));
    uart_printreg ("IntCtl  ", mfc0(12, 1));
    uart_printreg ("SRSCtl  ", mfc0(12, 2));
    uart_printreg ("Cause   ", mfc0(13, 0));
    uart_printreg ("PRId    ", mfc0(15, 0));
    uart_printreg ("EBase   ", mfc0(15, 1));
    uart_printreg ("CDMMBase", mfc0(15, 2));
    uart_printreg ("Config  ", mfc0(16, 0));
    uart_printreg ("Config1 ", mfc0(16, 1));
    uart_printreg ("Config2 ", mfc0(16, 2));
    uart_printreg ("Config3 ", mfc0(16, 3));
    uart_printreg ("Config4 ", mfc0(16, 4));
    uart_printreg ("Config5 ", mfc0(16, 5));
    uart_printreg ("Config7 ", mfc0(16, 7));
    uart_printreg ("WatchHi ", mfc0(19, 0));
    uart_printreg ("WatchHi1", mfc0(19, 1));
    uart_printreg ("WatchHi2", mfc0(19, 2));
    uart_printreg ("WatchHi3", mfc0(19, 3));
    uart_printreg ("Debug   ", mfc0(23, 0));
    uart_printreg ("PerfCtl0", mfc0(25, 0));
    uart_printreg ("PerfCtl1", mfc0(25, 2));
    uart_printreg ("DEVID   ", DEVID);
    uart_printreg ("OSCCON  ", OSCCON);
    uart_printreg ("DEVCFG0 ", DEVCFG0);
    uart_printreg ("DEVCFG1 ", DEVCFG1);
    uart_printreg ("DEVCFG2 ", DEVCFG2);
    uart_printreg ("DEVCFG3 ", DEVCFG3);

	stdlib_demo();

    while (1) {
        /* Invert pins PA7-PA0. */
        LATAINV = 1 << 0;  udelay (100000);
        LATAINV = 1 << 1;  udelay (100000);
        LATAINV = 1 << 2;  udelay (100000);
        LATAINV = 1 << 3;  udelay (100000);
        LATFINV = 1 << 13; udelay (100000);
        LATFINV = 1 << 12; udelay (100000);
        LATAINV = 1 << 6;  udelay (100000);
        LATAINV = 1 << 7;  udelay (100000);

        loop++;
        uart_putc ('.');
    }
}


void stdlib_demo(){
    // The only purpose of this function is to demonstrate
    // that the C standard library functions are available
    // and (probably) working as expected.

    // Simple kprintf.
    kprintf("=========================\nkprintf is working!\n");

    // Formatting
    kprintf("This is a number: %d - %x\n", 123456, 123456);
    // String rendering
    const char* stringA = "This is an example string!";
    char stringB[100];
    snprintf(stringB, 100, "Copied example: %s",stringA);
    kputs(stringB);

    // String functions:
    kprintf("Above text has length %zu.\n", strlen(stringB));

    // The limits are defined
    kprintf("INT_MAX is %d\n", INT_MAX);

}
