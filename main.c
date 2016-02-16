#include "pic32mz.h"
#include "uart_raw.h"
#include "global_config.h"
#include "interrupts.h"
#include "clock.h"
#include "kmem.h"
#include "context.h"
#include "libkern.h"

char str[] = "This is a global string!\n";
char empty[100]; /* This should land in .bss and get cleared by _start procedure. */

typedef struct cpuinfo {
    int tlb_entries;
    int ic_size;
    int ic_linesize;
    int ic_nways;
    int ic_nsets;
    int dc_size;
    int dc_linesize;
    int dc_nways;
    int dc_nsets;
} cpuinfo_t;

static cpuinfo_t cpuinfo;

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
void udelay (unsigned usec) {
  uint32_t now = mips32_get_c0(C0_COUNT);
  uint32_t final = now + usec * MHZ / 2;

  for (;;) {
    now = mips32_get_c0(C0_COUNT);

    /* This comparison is valid only when using a signed type. */
    if ((int) (now - final) >= 0)
      break;
  }
}

static void demo_ctx();

/*
 * Delays for at least the given number of milliseconds. May not be
 * nanosecond-accurate.
 */
void mdelay (unsigned msec) {
  unsigned now = clock_get_ms();
  unsigned final = now + msec;
  while (final > clock_get_ms());
}

static ctx_t ctx0, ctx1, ctx2;
static intptr_t stack1[200];
static intptr_t stack2[200];

static void demo_context_1() {
  int times = 3;

  kprintf("Context #1 has started.\n");

  do {
    ctx_switch(&ctx1, &ctx2);
    kprintf("Running in context #1.\n");
  } while (--times);

  ctx_switch(&ctx1, &ctx0);
}

static void demo_context_2() {
  int times = 3;

  kprintf("Context #2 has started.\n");

  do {
    ctx_switch(&ctx2, &ctx1);
    kprintf("Running in context #2.\n");
  } while (--times);

  // NOT REACHED
}

static void demo_ctx() {
  kprintf("Main context has started.\n");

  // Prepare alternative contexts
  register void *gp asm("$gp");
  ctx_init(&ctx1, demo_context_1, stack1 + 199, gp);
  ctx_init(&ctx2, demo_context_2, stack2 + 199, gp);

  // Switch to context 1
  ctx_switch(&ctx0, &ctx1);

  kprintf("Main context continuing.\n");
}

/* 
 * Read configuration register values, interpret and save them into the cpuinfo
 * structure for later use.
 */
static bool read_config() {
  uint32_t config0 = mips32_getconfig0();
  uint32_t cfg0_mt = config0 & CFG0_MT_MASK;
  char *cfg0_mt_str;

  if (cfg0_mt == CFG0_MT_TLB)
    cfg0_mt_str = "Standard TLB";
  else if (cfg0_mt == CFG0_MT_BAT)
    cfg0_mt_str = "BAT";
  else if (cfg0_mt == CFG0_MT_FIXED)
    cfg0_mt_str = "Fixed mapping";
  else if (cfg0_mt == CFG0_MT_DUAL)
    cfg0_mt_str = "Dual VTLB and FTLB";
  else
    cfg0_mt_str = "No MMU";

  kprintf("MMU Type: %s\n", cfg0_mt_str);

  /* CFG1 implemented? */
  if ((config0 & CFG0_M) == 0)
    return false;

  uint32_t config1 = mips32_getconfig1();

  /* FTLB or/and VTLB sizes */
  cpuinfo.tlb_entries = _mips32r2_ext(config1, CFG1_MMUS_SHIFT, CFG1_MMUS_BITS) + 1;

  /* Instruction cache size and organization. */
  cpuinfo.ic_linesize = (config1 & CFG1_IL_MASK) ? 16 : 0;
  cpuinfo.ic_nways = _mips32r2_ext(config1, CFG1_IA_SHIFT, CFG1_IA_BITS) + 1;
  cpuinfo.ic_nsets = 1 << (_mips32r2_ext(config1, CFG1_IS_SHIFT, CFG1_IS_BITS) + 6);
  cpuinfo.ic_size = cpuinfo.ic_nways * cpuinfo.ic_linesize * cpuinfo.ic_nsets;

  /* Data cache size and organization. */
  cpuinfo.dc_linesize = (config1 & CFG1_DL_MASK) ? 16 : 0;
  cpuinfo.dc_nways = _mips32r2_ext(config1, CFG1_DA_SHIFT, CFG1_DA_BITS) + 1;
  cpuinfo.dc_nsets = 1 << (_mips32r2_ext(config1, CFG1_DS_SHIFT, CFG1_DS_BITS) + 6);
  cpuinfo.dc_size = cpuinfo.dc_nways * cpuinfo.dc_linesize * cpuinfo.dc_nsets;

  kprintf("TLB Entries: %d\n", cpuinfo.tlb_entries);

  kprintf("Instruction cache:\n");
  kprintf(" - line size     : %d\n", cpuinfo.ic_linesize);
  kprintf(" - associativity : %d\n", cpuinfo.ic_nways);
  kprintf(" - sets per way  : %d\n", cpuinfo.ic_nsets);
  kprintf(" - size          : %d\n", cpuinfo.ic_size);

  kprintf("Data cache:\n");
  kprintf(" - line size     : %d\n", cpuinfo.dc_linesize);
  kprintf(" - associativity : %d\n", cpuinfo.dc_nways);
  kprintf(" - sets per way  : %d\n", cpuinfo.dc_nsets);
  kprintf(" - size          : %d\n", cpuinfo.dc_size);

  /* CFG2 implemented? */
  if ((config1 & CFG1_M) == 0)
    return false;

  uint32_t config2 = mips32_getconfig1();

  /* Config2 implemented? */
  if ((config2 & CFG2_M) == 0)
    return false;

  uint32_t config3 = mips32_getconfig3();

  kprintf("Small pages (1KiB) implemented : %s\n", (config3 & CFG3_SP) ? "yes" : "no");

  return true;
}

/*
 * Kernel Mode
 *
 * The processor is operating in Kernel Mode when the DM bit in the Debug
 * register is a zero, and any of the following three conditions is true:
 * - The KSU field in the CP0 Status register contains 0b00
 * - The EXL bit in the Status register is one
 * - The ERL bit in the Status register is one
 * The processor enters Kernel Mode at power-up, or as the result of an
 * interrupt, exception, or error. The processor leaves Kernel Mode and enters
 * User Mode when all of the previous three conditions are false, usually as
 * the result of an ERET instruction.
 */

/*
 * User Mode
 *
 * The processor is operating in User Mode when all of the following conditions
 * are true:
 * - The DM bit in the Debug register is a zero 
 * - The KSU field in the Status register contains 0b10
 * - The EXL and ERL bits in the Status register are both zero
 */

int kernel_main() {
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
  read_config();

  /* Demonstrate access to .data */
  kprintf ("%s", str);

  /* Test whether .bss appears to have been cleared. */
  char *p = empty;
  while (p < empty + sizeof(empty))
    if (*p++ != 0x00)
      kprintf("Apparently .bss was not cleared!\n");
  // TODO: Exit main? Ignore?

  /*
   * Print initial state of control registers.
   */
  kprintf ("-\n");
  kprintf ("Status   = 0x%08x\n", mips32_get_c0(C0_STATUS));
  kprintf ("IntCtl   = 0x%08x\n", mips32_get_c0(C0_INTCTL));
  kprintf ("SRSCtl   = 0x%08x\n", mips32_get_c0(C0_SRSCTL));
  kprintf ("Cause    = 0x%08x\n", mips32_get_c0(C0_CAUSE));
  kprintf ("PRId     = 0x%08x\n", mips32_get_c0(C0_PRID));
  kprintf ("EBase    = 0x%08x\n", mips32_get_c0(C0_EBASE));
  kprintf ("Config   = 0x%08x\n", mips32_get_c0(C0_CONFIG));
  kprintf ("Config1  = 0x%08x\n", mips32_get_c0(C0_CONFIG1));
  kprintf ("Config2  = 0x%08x\n", mips32_get_c0(C0_CONFIG2));
  kprintf ("Config3  = 0x%08x\n", mips32_get_c0(C0_CONFIG3));
  kprintf ("Config4  = 0x%08x\n", mips32_get_c0(C0_CONFIG4));
  kprintf ("Config5  = 0x%08x\n", mips32_get_c0(C0_CONFIG5));
  kprintf ("Debug    = 0x%08x\n", mips32_get_c0(C0_DEBUG));
  kprintf ("DEVID    = 0x%08x\n", DEVID);
  kprintf ("OSCCON   = 0x%08x\n", OSCCON);
  kprintf ("DEVCFG0  = 0x%08x\n", DEVCFG0);
  kprintf ("DEVCFG1  = 0x%08x\n", DEVCFG1);
  kprintf ("DEVCFG2  = 0x%08x\n", DEVCFG2);
  kprintf ("DEVCFG3  = 0x%08x\n", DEVCFG3);

  clock_init();

  demo_ctx();

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


