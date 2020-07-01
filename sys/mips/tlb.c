#include <mips/m32c0.h>
#include <mips/tlb.h>
#include <sys/interrupt.h>

#define mips32_getasid() (mips32_getentryhi() & PTE_ASID_MASK)
#define mips32_setasid(v) mips32_setentryhi((v)&PTE_ASID_MASK)

/* Prevents compiler from reordering load & store instructions. */
#define barrier() asm volatile("" ::: "memory")

static unsigned _tlb_size = 0;

/*
 * NOTE: functions that set coprocessor 0 registers like TLBHi/Lo, Index,
 * etc. must not be interrupted or generate exceptions between setting
 * these registers and executing the instruction that consumes them
 * (e.g. tlbwi), as any interrupt or exception can overwrite the contents
 * of these registers!
 */

static inline void _tlb_read(unsigned i, tlbentry_t *e) {
  mips32_setindex(i);
  mips32_tlbr();
  /*
   * Save the result into registers first. If we wrote it directly to memory,
   * we could generate an exception and overwrite the result!
   */
  tlbhi_t hi = mips32_getentryhi();
  tlblo_t lo0 = mips32_getentrylo0();
  tlblo_t lo1 = mips32_getentrylo1();
  barrier();
  *e = (tlbentry_t){.hi = hi, .lo0 = lo0, .lo1 = lo1};
}

static inline void _load_tlb_entry(tlbentry_t *e) {
  /*
   * Again, we don't want to generate exceptions after setting
   * EntryHi, so we first load the entry from memory to registers.
   */
  tlbhi_t hi = e->hi;
  tlblo_t lo0 = e->lo0;
  tlblo_t lo1 = e->lo1;
  barrier();
  mips32_setentryhi(hi);
  mips32_setentrylo0(lo0);
  mips32_setentrylo1(lo1);
}

static inline int _tlb_probe(tlbhi_t hi) {
  mips32_setentryhi(hi);
  mips32_tlbp();
  return mips32_getindex();
}

static inline void _tlb_invalidate(unsigned i) {
  mips32_setindex(i);
  mips32_setentryhi(0);
  mips32_setentrylo0(0);
  mips32_setentrylo1(0);
  mips32_tlbwi();
}

static void read_tlb_size(void) {
  uint32_t cfg0 = mips32_getconfig0();
  if ((cfg0 & CFG0_MT_MASK) != CFG0_MT_TLB)
    return;
  if (!(cfg0 & CFG0_M))
    return;

  uint32_t cfg1 = mips32_getconfig1();
  _tlb_size = ((cfg1 & CFG1_MMUS_MASK) >> CFG1_MMUS_SHIFT) + 1;
}

/* TLB has been almost completely initialized by "mips_init",
 * so not much is happening here. */
void init_mips_tlb(void) {
  read_tlb_size();
  /* We're not going to use C0_CONTEXT so set it to zero. */
  mips32_setcontext(0);
}

void tlb_invalidate(tlbhi_t hi) {
  SCOPED_INTR_DISABLED();
  tlbhi_t saved = mips32_getasid();
  int i = _tlb_probe(hi);
  if (i >= 0)
    _tlb_invalidate(i);
  mips32_setasid(saved);
}

void tlb_invalidate_asid(tlbhi_t asid) {
  SCOPED_INTR_DISABLED();
  tlbhi_t saved = mips32_getasid();
  for (unsigned i = mips32_getwired(); i < _tlb_size; i++) {
    tlbentry_t e;
    _tlb_read(i, &e);
    /* Ignore global mappings! */
    if ((e.lo0 & PTE_GLOBAL) && (e.lo1 & PTE_GLOBAL))
      continue;
    /* Ignore mappings with different ASID */
    if ((e.hi & PTE_ASID_MASK) != asid)
      continue;
    _tlb_invalidate(i);
  }
  mips32_setasid(saved);
}

void tlb_write(unsigned i, tlbentry_t *e) {
  SCOPED_INTR_DISABLED();
  tlbhi_t saved = mips32_getasid();
  _load_tlb_entry(e);
  if (i == TLBI_RANDOM) {
    mips32_tlbwr();
  } else {
    mips32_setindex(i);
    mips32_tlbwi();
  }
  mips32_setasid(saved);
}
