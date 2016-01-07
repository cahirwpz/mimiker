/*
 * Include processor definitions.
 */
#include "pic32mz.h"
#include "uart_raw.h"
#include "common.h"
#include "global_config.h"
#include "interrupts.h"
#include "clock.h"
#include "bitmap.h"

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

void read_config()
{
    int config = mfc0(16, 0);
    kprintf ("\nConfig = 0x%08x\n", config);
    
    // Default [0 = false, 1 = true]
    
    kprintf("Kseg2 and kseg3 [2,7-Uncached]: %d\n", BITS_GET(config, 3, 28) ); // ref. to MD00942 to Table 7.37 Cache Coherency Attributes
    kprintf("Kuseg and useg [2,7-Uncached]: %d\n", BITS_GET(config, 3, 25) ); // ref. to MD00942 to Table 7.37 Cache Coherency Attributes
    kprintf("Instruction ScratchPad is present: %d\n", BIT_GET(config,24) );
    kprintf("Data ScratchPad is present: %d\n", BIT_GET(config,23) );
    kprintf("User Defined Instruction are implemented: %d\n", BIT_GET(config,22) );
    kprintf("SimpleBE bus mode is enabled: %d\n", BIT_GET(config,21) );
    kprintf("Type of MDU [0-Fast, 1-Iterative]: %d\n", BIT_GET(config,20) );
    kprintf("Diagnostic bit: %d\n", BIT_GET(config,19) ); // ref. to MD00213 "Cache Configuration Application Note"
    kprintf("Merging [0-No, 2-Allowed, 1,3-Reserved]: %d\n", BITS_GET(config, 2, 17) );
    kprintf("Type of MDU [0-Fast, 1-Iterative]: %d\n", BIT_GET(config,16) );
    kprintf("Endian mode [0-Little, 1-Big]: %d\n", BIT_GET(config,15) );
    kprintf("Architecture type [00-MIPS32]: %d\n", BITS_GET(config, 2, 13) );
    kprintf("Architecture revision lev. [0-Release1, 1-Release2, 2-7-Reserved]: %d\n", BITS_GET(config, 3, 10) );
    kprintf("MMU type [1-Standard TLB, 3-Fixed mapping, 0,2,4-7-Reserved]: %d\n", BITS_GET(config, 3, 7) );
    kprintf("Zeros: %d\n", BITS_GET(config, 4, 3) );
    kprintf("Kseg0 [2,7-Uncached]: %d\n", BITS_GET(config, 3, 0) ); // ref. to MD00942 to Table 7.37 Cache Coherency Attributes
    
    if( (config & (1<<31)) == 0 )
        return;
    
    int config1 = mfc0(16, 1);
    kprintf ("\nConfig1 = 0x%08x\n", config1);

    kprintf("Number of entries in TLB (minus 1): %d\n", BITS_GET(config1, 6, 25) );    
    kprintf("Number of instruction cache sets per way [0-64, 1-128, 2-256, 3-512, 4-1024, 5-7-Reserved]: %d\n", BITS_GET(config1, 3, 22) );    
    kprintf("Instruction cache line size: [0-No instr. cache, 3-16 bytes, 1,2,4-7 - Reserved]: %d\n", BITS_GET(config1, 3, 19) );
    kprintf("Level of instruction cache associativity [0-Direct mapped, 1-2way, 2-3way, 3-4way, 4-7-Reserved]: %d\n", BITS_GET(config1, 3, 16) );    
    kprintf("Number of data cache sets per way [0-64, 1-128, 2-256, 3-512, 4-1024, 5-7-Reserved]: %d\n", BITS_GET(config1, 3, 13) );    
    kprintf("data cache line size: [0-No data cache, 3-16 bytes, 1,2,4-7 - Reserved]: %d\n", BITS_GET(config1, 3, 10) );
    kprintf("Type of the data cache [0-Direct mapped, 1-2way, 2-3way, 3-4way, 4-7-Reserved]: %d\n", BITS_GET(config1, 3, 7) ); 
    kprintf("Coprocesor 2 present: %d\n", BIT_GET(config1, 6) );
    kprintf("MDMX is implemented: %d\n", BIT_GET(config1, 5) );
    kprintf("Peformance Counter register is implemented: %d\n", BIT_GET(config1, 4) );
    kprintf("One or more Watch registers are implemented: %d\n", BIT_GET(config1, 3) );
    kprintf("MIPS16e (code compression) is implemented: %d\n", BIT_GET(config1, 2) );
    kprintf("EJTAG is implemented: %d\n", BIT_GET(config1, 1) );
    kprintf("FPU is implemented: %d\n", BIT_GET(config1, 0) );
    
    if( (config1 & (1<<31)) == 0 )
        return;
    
    int config2 = mfc0(16, 2);
    kprintf ("\nConfig2 = 0x%08x\n", config2);
    kprintf ("Bits 31:0 are reserved\n");
    
    if( (config2 & (1<<31)) == 0 )
        return;

    int config3 = mfc0(16, 3);
    kprintf ("\nConfig3 = 0x%08x\n", config3);
          
    kprintf("Zeros: %d\n", BITS_GET(config3, 8, 23) );
    kprintf("[0-IPL and RIPL filds are 6-bits, 1-...8-bits, 2,3-Reserved]: %d\n", BITS_GET(config3, 2, 21) );
    kprintf("microMIPS Architecture revision lev. [0-Release1, 1-7-Reserved]: %d\n", BITS_GET(config3, 3, 18) );
    kprintf("MCU ASE is implemented %d\n", BIT_GET(config3, 17) );
    kprintf("Vectoring to an exception [0-MIPS32 ISA, 1-microMIPS]: %d\n", BIT_GET(config3, 16) );
    kprintf("ISA [0-Only MIPS32, 1-Only microMIPS, 2-Both implem. MIPS32 ISA used when coming out of reset, 3-Both implem. MicroMips used ...]: %d\n", BITS_GET(config3, 2, 14) );
    kprintf("UserLocal register is implemented: %d\n", BIT_GET(config3, 13) );
    kprintf("The RIE and XIE bits are implemented within the PageGrain: %d\n", BIT_GET(config3, 12) );
    kprintf("Revision 2 of the MIPS DSP module is implemented: %d\n", BIT_GET(config3, 11) );
    kprintf("MIPS DSP module extension is implemented: %d\n", BIT_GET(config3, 10) );
    kprintf("Zero: %d\n", BIT_GET(config3, 9) );
    
    kprintf("iFlowTrace hardware is present: %d\n", BIT_GET(config3, 8) );
    kprintf("Large physical address support is implemented: %d\n", BIT_GET(config3, 7) );
    kprintf("Support for EIC interrupt mode is implemented: %d\n", BIT_GET(config3, 6) );
    kprintf("Vectored interrupts are implemented: %d\n", BIT_GET(config3, 5) );
    kprintf("Small page support is implemented: %d\n", BIT_GET(config3, 4) );
    kprintf("Common Device Memory Map is implemented: %d\n", BIT_GET(config3, 3) );
    kprintf("Zero: %d\n", BIT_GET(config3, 2) );
    kprintf("SmartMIPS ASE is implemented: %d\n", BIT_GET(config3, 1) );
    kprintf("Trace logic is implemented: %d\n", BIT_GET(config3, 0) );
    
    if( (config3 & (1<<31)) == 0 )
        return;
   
    int config4 = mfc0(16, 4);
    kprintf ("\nConfig4 = 0x%08x\n", config4);
     
    kprintf("Zeros: %d\n", BITS_GET(config4, 30, 0) );
    
    if( (config4 & (1<<31)) == 0 )
        return;
    
    int config5 = mfc0(16, 5);
    kprintf ("\nConfig5 = 0x%08x\n", config5);
    kprintf("Config5 is present (?) (Should read as 0): %d\n", BIT_GET(config5, 31));
    kprintf("Zeros: %d\n", BITS_GET(config5, 28, 3) );
    kprintf("User mode FR instructions allowed: %d\n", BIT_GET(config5, 2));
    kprintf("Zero: %d\n", BIT_GET(config5, 1));
    kprintf("Nested Fault feature is present: %d\n", BIT_GET(config5, 0));

    //How do we know that config7 is present? 

    int config7 = mfc0(16, 7);
    kprintf ("\nConfig7 = 0x%08x\n", config7);
    kprintf("Wait IE Ignore: %d\n", BIT_GET(config7, 31));
    kprintf("Zeros: %d\n", BITS_GET(config7, 12, 19) );
    kprintf("Hardware Cache Initialization: %d\n", BIT_GET(config7, 18));
    kprintf("Zeros: %d\n", BITS_GET(config7, 18, 0) );

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

    read_config();

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
