#include <common.h>
#include <stdc.h>
#include <mips/mips.h>
#include <mips/cpuinfo.h>

cpuinfo_t cpuinfo;

/*
 * Read configuration register values, interpret and save them into the cpuinfo
 * structure for later use.
 */
static bool cpu_read_config() {
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
  cpuinfo.tlb_entries =
    _mips32r2_ext(config1, CFG1_MMUS_SHIFT, CFG1_MMUS_BITS) + 1;

  /* Instruction cache size and organization. */
  cpuinfo.ic_linesize = (config1 & CFG1_IL_MASK) ? 32 : 0;
  cpuinfo.ic_nways = _mips32r2_ext(config1, CFG1_IA_SHIFT, CFG1_IA_BITS) + 1;
  cpuinfo.ic_nsets =
    1 << (_mips32r2_ext(config1, CFG1_IS_SHIFT, CFG1_IS_BITS) + 6);
  cpuinfo.ic_size = cpuinfo.ic_nways * cpuinfo.ic_linesize * cpuinfo.ic_nsets;

  /* Data cache size and organization. */
  cpuinfo.dc_linesize = (config1 & CFG1_DL_MASK) ? 32 : 0;
  cpuinfo.dc_nways = _mips32r2_ext(config1, CFG1_DA_SHIFT, CFG1_DA_BITS) + 1;
  cpuinfo.dc_nsets =
    1 << (_mips32r2_ext(config1, CFG1_DS_SHIFT, CFG1_DS_BITS) + 6);
  cpuinfo.dc_size = cpuinfo.dc_nways * cpuinfo.dc_linesize * cpuinfo.dc_nsets;

  kprintf("TLB Entries: %d\n", cpuinfo.tlb_entries);

  kprintf("I-cache: %d KiB, %d-way associative, line size: %d bytes\n",
          cpuinfo.ic_size / 1024, cpuinfo.ic_nways, cpuinfo.ic_linesize);
  kprintf("D-cache: %d KiB, %d-way associative, line size: %d bytes\n",
          cpuinfo.dc_size / 1024, cpuinfo.dc_nways, cpuinfo.dc_linesize);

  /* Config2 implemented? */
  if ((config1 & CFG1_M) == 0)
    return false;

  uint32_t config2 = mips32_getconfig1();

  /* Config3 implemented? */
  if ((config2 & CFG2_M) == 0)
    return false;

  uint32_t config3 = mips32_getconfig3();

  kprintf("Vectored interrupts implemented : %s\n",
          (config3 & CFG3_VI) ? "yes" : "no");
  kprintf("EIC interrupt mode implemented  : %s\n",
          (config3 & CFG3_VEIC) ? "yes" : "no");
  kprintf("UserLocal register implemented  : %s\n",
          (config3 & CFG3_ULRI) ? "yes" : "no");

  return true;
}

void cpu_sr_dump() {
  unsigned sr = mips32_get_c0(C0_STATUS);
  static char *mode[] = {"kernel", "supervisor", "user"};
  static char *boolean[] = {"no", "yes"};

  kprintf("Status register :\n"
          " - FPU enabled        : %s\n"
          " - Interrupt mask     : %02x\n"
          " - Operating mode     : %s\n"
          " - Exception level    : %s\n"
          " - Interrupts enabled : %s\n",
          boolean[(sr & SR_CU1) >> SR_CU1_SHIFT],
          (sr & SR_IMASK) >> SR_IMASK_SHIFT,
          mode[(sr & SR_KSU_MASK) >> SR_KSU_SHIFT],
          boolean[(sr & SR_EXL) >> SR_EXL_SHIFT],
          boolean[(sr & SR_IE) >> SR_IE_SHIFT]);
}

/* Print state of control registers. */
static void cpu_dump() {
  unsigned cr = mips32_get_c0(C0_CAUSE);
  unsigned intctl = mips32_get_c0(C0_INTCTL);
  unsigned srsctl = mips32_get_c0(C0_SRSCTL);

  kprintf("Cause    : TI:%d IV:%d IP:%d ExcCode:%d\n",
          (cr & CR_TI) >> CR_TI_SHIFT, (cr & CR_IV) >> CR_IV_SHIFT,
          (cr & CR_IP_MASK) >> CR_IP_SHIFT, (cr & CR_X_MASK) >> CR_X_SHIFT);
  cpu_sr_dump();
  kprintf("IntCtl   : IPTI:%d IPPCI:%d VS:%d\n",
          (intctl & INTCTL_IPTI) >> INTCTL_IPTI_SHIFT,
          (intctl & INTCTL_IPPCI) >> INTCTL_IPPCI_SHIFT,
          (intctl & INTCTL_VS) >> INTCTL_VS_SHIFT);
  kprintf("SrsCtl   : HSS:%d\n", (srsctl & SRSCTL_HSS) >> SRSCTL_HSS_SHIFT);
  kprintf("EPC      : $%08x\n", (unsigned)mips32_get_c0(C0_EPC));
  kprintf("ErrPC    : $%08x\n", (unsigned)mips32_get_c0(C0_ERRPC));
  kprintf("EBase    : $%08x\n", (unsigned)mips32_get_c0(C0_EBASE));
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

void cpu_init() {
  cpu_read_config();
  cpu_dump();
}
