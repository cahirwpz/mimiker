/*
 * Include processor definitions.
 */
#include "pic32mz.h"

#define MHZ     200             /* CPU clock. */

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

/*
 * Boot code at bfc00000.
 * Setup stack pointer and $gp registers, and jump to main().
 */
asm ("          .section .exception");
asm ("          .globl _start");
asm ("          .type _start, function");
asm ("_start:   la      $sp, _estack");
asm ("          la      $ra, main");
asm ("          la      $gp, _gp");
asm ("          jr      $ra");
asm ("          .text");
asm ("_estack   = _end + 0x1000");

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
 * Send a byte to the UART transmitter, with interrupts disabled.
 */
void putch (unsigned char c)
{
    /* Wait for transmitter shift register empty. */
    while (! (U1STA & PIC32_USTA_TRMT))
        continue;

again:
    /* Send byte. */
    U1TXREG = c;

    /* Wait for transmitter shift register empty. */
    while (! (U1STA & PIC32_USTA_TRMT))
        continue;

    if (c == '\n') {
        c = '\r';
        goto again;
    }
}

/*
 * Wait for the byte to be received and return it.
 */
unsigned getch (void)
{
    unsigned c;

    for (;;) {
        /* Wait until receive data available. */
        if (U1STA & PIC32_USTA_URXDA) {
            c = (unsigned char) U1RXREG;
            break;
        }
    }
    return c;
}

int hexchar (unsigned val)
{
    return "0123456789abcdef" [val & 0xf];
}

void printreg (const char *p, unsigned val)
{
    for (; *p; ++p)
        putch (*p);

    putch ('=');
    putch (' ');
    putch (hexchar (val >> 28));
    putch (hexchar (val >> 24));
    putch (hexchar (val >> 20));
    putch (hexchar (val >> 16));
    putch (hexchar (val >> 12));
    putch (hexchar (val >> 8));
    putch (hexchar (val >> 4));
    putch (hexchar (val));
    putch ('\n');
}

int main()
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
    U1RXR = 2;                          /* assign UART1 receive to pin RPF4 */
    RPF5R = 1;                          /* assign pin RPF5 to UART1 transmit */

    U1BRG = PIC32_BRG_BAUD (MHZ * 500000, 115200);
    U1STA = 0;
    U1MODE = PIC32_UMODE_PDSEL_8NPAR |      /* 8-bit data, no parity */
             PIC32_UMODE_ON;                /* UART Enable */
    U1STASET = PIC32_USTA_URXEN |           /* Receiver Enable */
               PIC32_USTA_UTXEN;            /* Transmit Enable */

    /*
     * Print initial state of control registers.
     */
    putch ('-');
    putch ('\n');
    printreg ("Status  ", mfc0(12, 0));
    printreg ("IntCtl  ", mfc0(12, 1));
    printreg ("SRSCtl  ", mfc0(12, 2));
    printreg ("Cause   ", mfc0(13, 0));
    printreg ("PRId    ", mfc0(15, 0));
    printreg ("EBase   ", mfc0(15, 1));
    printreg ("CDMMBase", mfc0(15, 2));
    printreg ("Config  ", mfc0(16, 0));
    printreg ("Config1 ", mfc0(16, 1));
    printreg ("Config2 ", mfc0(16, 2));
    printreg ("Config3 ", mfc0(16, 3));
    printreg ("Config4 ", mfc0(16, 4));
    printreg ("Config5 ", mfc0(16, 5));
    printreg ("Config7 ", mfc0(16, 7));
    printreg ("WatchHi ", mfc0(19, 0));
    printreg ("WatchHi1", mfc0(19, 1));
    printreg ("WatchHi2", mfc0(19, 2));
    printreg ("WatchHi3", mfc0(19, 3));
    printreg ("Debug   ", mfc0(23, 0));
    printreg ("PerfCtl0", mfc0(25, 0));
    printreg ("PerfCtl1", mfc0(25, 2));
    printreg ("DEVID   ", DEVID);
    printreg ("OSCCON  ", OSCCON);
    printreg ("DEVCFG0 ", DEVCFG0);
    printreg ("DEVCFG1 ", DEVCFG1);
    printreg ("DEVCFG2 ", DEVCFG2);
    printreg ("DEVCFG3 ", DEVCFG3);

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
        putch ('.');
    }
}
