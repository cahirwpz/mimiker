#include <mips/mips.h>
#include <mips/pmap.h>
#include <mips/tlb.h>
#include <sys/mimiker.h>
#include <sys/interrupt.h>

#define mips32_getasid() (mips32_getentryhi() & PTE_ASID_MASK)
#define mips32_setasid(v) mips32_setentryhi((v)&PTE_ASID_MASK)

/* Prevents compiler from reordering load & store instructions. */
#define barrier() asm volatile("" ::: "memory")

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

static inline void _tlb_write(unsigned i, tlbentry_t *e) {
  _load_tlb_entry(e);
  mips32_setindex(i);
  mips32_tlbwi();
}

static inline void _tlb_write_random(tlbentry_t *e) {
  _load_tlb_entry(e);
  mips32_tlbwr();
}

static inline int _tlb_probe(tlbhi_t hi) {
  mips32_setentryhi(hi);
  mips32_tlbp();
  return mips32_getindex();
}

static inline void _tlb_invalidate(unsigned i) {
  static tlbentry_t invalid = {.hi = 0, .lo0 = 0, .lo1 = 0};
  _tlb_write(i, &invalid);
}

static unsigned _tlb_size = 0;

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
void tlb_init(void) {
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

void tlb_invalidate_all(void) {
  SCOPED_INTR_DISABLED();
  tlbhi_t saved = mips32_getasid();
  for (unsigned i = mips32_getwired(); i < _tlb_size; i++)
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

void tlb_read(unsigned i, tlbentry_t *e) {
  SCOPED_INTR_DISABLED();
  tlbhi_t saved = mips32_getasid();
  _tlb_read(i, e);
  mips32_setasid(saved);
}

void tlb_write(unsigned i, tlbentry_t *e) {
  SCOPED_INTR_DISABLED();
  tlbhi_t saved = mips32_getasid();
  if (i == TLBI_RANDOM)
    _tlb_write_random(e);
  else
    _tlb_write(i, e);
  mips32_setasid(saved);
}

void tlb_overwrite_random(tlbentry_t *e) {
  SCOPED_INTR_DISABLED();
  tlbhi_t saved = mips32_getasid();
  int i = _tlb_probe(e->hi);
  if (i >= 0)
    _tlb_write(i, e);
  else
    _tlb_write_random(e);
  mips32_setasid(saved);
}

int tlb_probe(tlbentry_t *e) {
  SCOPED_INTR_DISABLED();
  tlbhi_t saved = mips32_getasid();
  int i = _tlb_probe(e->hi);
  if (i >= 0)
    _tlb_read(i, e);
  mips32_setasid(saved);
  return i;
}

unsigned tlb_size(void) {
  return _tlb_size;
}
