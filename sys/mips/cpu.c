#define KL_LOG KL_INIT
#include <sys/klog.h>
#include <stdbool.h>
#include <mips/m32c0.h>
#include <mips/cpuinfo.h>

cpuinfo_t cpuinfo;

#define bitfield(value, field)                                                 \
  (((value) >> field##_SHIFT) & ((1L << field##_BITS) - 1))
#define bit(value, field) (((value) >> field##_SHIFT) & 1)

static inline const char *boolean(int b) {
  return b ? "yes" : "no";
}

/*
 * Read configuration register values, interpret and save them into the cpuinfo
 * structure for later use.
 */
static void cpu_read_config(void) {
  uint32_t config0 = mips32_getconfig0();
  uint32_t cfg0_mt = config0 & CFG0_MT_MASK;
  char *cfg0_mt_str;

  klog("MIPS32 Architecture Release %d", bitfield(config0, CFG0_AR) + 1);

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

  klog("MMU Type: %s", cfg0_mt_str);

  /* CFG1 implemented? */
  if ((config0 & CFG0_M) == 0)
    return;

  uint32_t cfg1 = mips32_getconfig1();

  klog("FPU implemented: %s", boolean(cfg1 & CFG1_FP));
  klog("Watch registers implemented: %s", boolean(cfg1 & CFG1_WR));
  klog("Performance Counters registers implemented: %s",
       boolean(cfg1 & CFG1_PC));

  /* FTLB or/and VTLB sizes */
  cpuinfo.tlb_entries = bitfield(cfg1, CFG1_MMUS) + 1;

  /* Instruction cache size and organization. */
  cpuinfo.ic_linesize = (cfg1 & CFG1_IL_MASK) ? 32 : 0;
  cpuinfo.ic_nways = bitfield(cfg1, CFG1_IA) + 1;
  cpuinfo.ic_nsets = 1 << (bitfield(cfg1, CFG1_IS) + 6);
  cpuinfo.ic_size = cpuinfo.ic_nways * cpuinfo.ic_linesize * cpuinfo.ic_nsets;

  /* Data cache size and organization. */
  cpuinfo.dc_linesize = (cfg1 & CFG1_DL_MASK) ? 32 : 0;
  cpuinfo.dc_nways = bitfield(cfg1, CFG1_DA) + 1;
  cpuinfo.dc_nsets = 1 << (bitfield(cfg1, CFG1_DS) + 6);
  cpuinfo.dc_size = cpuinfo.dc_nways * cpuinfo.dc_linesize * cpuinfo.dc_nsets;

  klog("TLB Entries: %d", cpuinfo.tlb_entries);

  klog("I-cache: %d KiB, %d-way associative, line size: %d bytes",
       cpuinfo.ic_size / 1024, cpuinfo.ic_nways, cpuinfo.ic_linesize);
  klog("D-cache: %d KiB, %d-way associative, line size: %d bytes",
       cpuinfo.dc_size / 1024, cpuinfo.dc_nways, cpuinfo.dc_linesize);

  /* Config2 implemented? */
  if ((cfg1 & CFG1_M) == 0)
    return;

  uint32_t cfg2 = mips32_getconfig2();

  /* Config3 implemented? */
  if ((cfg2 & CFG2_M) == 0)
    return;

  uint32_t cfg3 = mips32_getconfig3();

  klog("BadInstrP register implemented: %s", boolean(cfg3 & CFG3_BP));
  klog("BadInstr register implemented: %s", boolean(cfg3 & CFG3_BI));
  klog("Segment Control implemented: %s", boolean(cfg3 & CFG3_SC));
  klog("UserLocal register implemented: %s", boolean(cfg3 & CFG3_ULRI));
  klog("Read / Execute Inhibit bits implemented: %s", boolean(cfg3 & CFG3_RXI));
  klog("EIC interrupt mode implemented: %s", boolean(cfg3 & CFG3_VEIC));
  klog("Vectored interrupts implemented: %s", boolean(cfg3 & CFG3_VI));

  /* Config4 implemented? */
  if ((cfg3 & CFG3_M) == 0)
    return;

  uint32_t cfg4 = mips32_getconfig4();

  klog("TLBINV / TLBINVF supported: %s", boolean(bitfield(cfg4, CFG4_IE)));

  /* Config5 implemented? */
  if ((cfg4 & CFG4_M) == 0)
    return;

  uint32_t cfg5 = mips32_getconfig5();

  klog("Exhanced Virtual Address extension: %s", boolean(cfg5 & CFG5_EVA));
}

void cpu_sr_dump(void) {
  unsigned sr = mips32_get_c0(C0_STATUS);
  static char *mode[] = {"kernel", "supervisor", "user"};

  klog("Status register :");
  klog(" - FPU enabled        : %s", boolean(sr & SR_CU1));
  klog(" - Interrupt mask     : %02x", bitfield(sr, SR_IMASK));
  klog(" - Operating mode     : %s", mode[bitfield(sr, SR_KSU)]);
  klog(" - Exception level    : %s", boolean(sr & SR_EXL));
  klog(" - Interrupts enabled : %s", boolean(sr & SR_IE));
}

/* Print state of control registers. */
static void cpu_dump(void) {
  unsigned cr = mips32_get_c0(C0_CAUSE);
  unsigned intctl = mips32_get_c0(C0_INTCTL);
  unsigned srsctl = mips32_get_c0(C0_SRSCTL);

  klog("Cause    : TI:%d IV:%d IP:%d ExcCode:%d", bit(cr, CR_TI),
       bit(cr, CR_IV), bitfield(cr, CR_IP), bitfield(cr, CR_X));
  cpu_sr_dump();
  klog("IntCtl   : IPTI:%d IPPCI:%d VS:%d", bitfield(intctl, INTCTL_IPTI),
       bitfield(intctl, INTCTL_IPPCI), bitfield(intctl, INTCTL_VS));
  klog("SrsCtl   : HSS:%d", bitfield(srsctl, SRSCTL_HSS));
  klog("EPC      : $%08x", (unsigned)mips32_get_c0(C0_EPC));
  klog("ErrPC    : $%08x", (unsigned)mips32_get_c0(C0_ERRPC));
  klog("EBase    : $%08x", (unsigned)mips32_get_c0(C0_EBASE));
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

void init_mips_cpu(void) {
  cpu_read_config();
  cpu_dump();
}
