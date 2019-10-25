#include <mips/mips.h>
#include <mips/tlb.h>
#include <sys/mimiker.h>

extern const char _ebase[];
extern uint8_t __bss[];
extern uint8_t __ebss[];

__boot_text static void halt(void) {
  for (;;)
    continue;
}

__boot_text void mips_init(void) {
  /*
   * Ensure we're in kernel mode, disable FPU,
   * leave error level & exception level and disable interrupts.
   */
  mips32_bc_c0(C0_STATUS,
               SR_IPL_MASK | SR_KSU_MASK | SR_CU1 | SR_ERL | SR_EXL | SR_IE);

  /* Clear pending software interrupts */
  mips32_bc_c0(C0_CAUSE, CR_IP_MASK);

  /*
   * Enable Vectored Interrupt Mode as described in „MIPS32® 24KETM Processor
   * Core Family Software User’s Manual”, chapter 6.3.1.2.
   */

  /* The location of exception vectors is set to EBase. */
  mips32_set_c0(C0_EBASE, _ebase);
  mips32_bc_c0(C0_STATUS, SR_BEV);
  /* Use the special interrupt vector at EBase + 0x200. */
  mips32_bs_c0(C0_CAUSE, CR_IV);
  /* Set vector spacing to 0. */
  mips32_set_c0(C0_INTCTL, INTCTL_VS_0);

  /* Clear BSS section using physical addresses. */
  uint32_t *ptr = (uint32_t *)__bss;
  uint32_t *end = (uint32_t *)__ebss;
  while (ptr < end)
    *ptr++ = 0;

  /*
   * Clear all entries in TLB.
   */
  if ((mips32_getconfig0() & CFG0_MT_MASK) != CFG0_MT_TLB)
    halt();

  unsigned nentries =
    ((mips32_getconfig1() & CFG1_MMUS_MASK) >> CFG1_MMUS_SHIFT) + 1;

  mips32_setentryhi(0);
  mips32_setentrylo0(0);
  mips32_setentrylo1(0);
  for (unsigned i = 0; i < nentries; i++) {
    mips32_setindex(i);
    mips32_tlbwi();
  }
}

/* Following code is used by gdb scripts. */

/* Compiler does not know that debugger (external agent) will read
 * the structure and will remove it and optimize out all references to it.
 * Hence it has to be marked with `volatile`. */
static __boot_data volatile tlbentry_t _gdb_tlb_entry;

__boot_text unsigned _gdb_tlb_size(void) {
  return ((mips32_getconfig1() & CFG1_MMUS_MASK) >> CFG1_MMUS_SHIFT) + 1;
}

/* Fills _gdb_tlb_entry structure with TLB entry. */
__boot_text void _gdb_tlb_read_index(unsigned i) {
  tlbhi_t saved = mips32_getentryhi();
  mips32_setindex(i);
  mips32_tlbr();
  _gdb_tlb_entry = (tlbentry_t){.hi = mips32_getentryhi(),
                                .lo0 = mips32_getentrylo0(),
                                .lo1 = mips32_getentrylo1()};
  mips32_setentryhi(saved);
}
