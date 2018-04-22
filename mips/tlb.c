#include <mips/mips.h>
#include <mips/tlb.h>
#include <vm.h>

#define mips32_getasid() (mips32_getentryhi() & PTE_ASID_MASK)
#define mips32_setasid(v) mips32_setentryhi((v)&PTE_ASID_MASK)

static inline void _tlb_read(unsigned i, tlbentry_t *e) {
  mips32_setindex(i);
  asm volatile("tlbr; ehb" : : : "memory");
  *e = (tlbentry_t){.hi = mips32_getentryhi(),
                    .lo0 = mips32_getentrylo0(),
                    .lo1 = mips32_getentrylo1()};
}

static inline void _tlb_write(unsigned i, tlbentry_t *e) {
  mips32_setentryhi(e->hi);
  mips32_setentrylo0(e->lo0);
  mips32_setentrylo1(e->lo1);
  mips32_setindex(i);
  asm volatile("tlbwi; ehb" : : : "memory");
}

static inline void _tlb_write_random(tlbentry_t *e) {
  mips32_setentryhi(e->hi);
  mips32_setentrylo0(e->lo0);
  mips32_setentrylo1(e->lo1);
  asm volatile("tlbwr; ehb" : : : "memory");
}

static inline int _tlb_probe(tlbhi_t hi) {
  mips32_setentryhi(hi);
  asm volatile("tlbp; ehb" : : : "memory");
  return mips32_getindex();
}

static inline void _tlb_invalidate(unsigned i) {
  static tlbentry_t invalid = {.hi = 0, .lo0 = 0, .lo1 = 0};

  _tlb_write(i, &invalid);
}

static unsigned _tlb_size = 0;

static void read_tlb_size(void) {
  uint32_t config0 = mips32_getconfig0();
  if ((config0 & CFG0_MT_MASK) != CFG0_MT_TLB)
    return;
  if (!(config0 & CFG0_M))
    return;

  uint32_t config1 = mips32_getconfig1();
  _tlb_size = _mips32r2_ext(config1, CFG1_MMUS_SHIFT, CFG1_MMUS_BITS) + 1;
}

void tlb_init(void) {
  read_tlb_size();
  if (tlb_size() == 0)
    panic("No TLB detected!");

  tlb_invalidate_all();
  /* Shift C0_CONTEXT left, because we shift it right in tlb_refill_handler.
   * This is little hack to make page table sized 4MB, but causes us to
   * keep PTE in KSEG2. */
  mips32_setcontext(PT_BASE << 1);
  /* First wired TLB entry is shared between kernel-PDE and user-PDE. */
  mips32_setwired(1);
}

void tlb_invalidate(tlbhi_t hi) {
  tlbhi_t saved = mips32_getasid();
  int i = _tlb_probe(hi);
  if (i >= 0)
    _tlb_invalidate(i);
  mips32_setasid(saved);
}

void tlb_invalidate_all(void) {
  tlbhi_t saved = mips32_getasid();
  for (unsigned i = mips32_getwired(); i < tlb_size(); i++)
    _tlb_invalidate(i);
  mips32_setasid(saved);
}

void tlb_invalidate_asid(tlbhi_t invalid) {
  tlbhi_t saved = mips32_getasid();
  for (unsigned i = mips32_getwired(); i < tlb_size(); i++) {
    tlbentry_t e;
    _tlb_read(i, &e);
    if ((e.hi & PTE_ASID_MASK) == invalid)
      _tlb_invalidate(i);
  }
  mips32_setasid(saved);
}

void tlb_read(unsigned i, tlbentry_t *e) {
  tlbhi_t saved = mips32_getasid();
  _tlb_read(i, e);
  mips32_setasid(saved);
}

void tlb_write(unsigned i, tlbentry_t *e) {
  tlbhi_t saved = mips32_getasid();
  if (i == TLBI_RANDOM)
    _tlb_write_random(e);
  else
    _tlb_write(i, e);
  mips32_setasid(saved);
}

void tlb_overwrite_random(tlbentry_t *e) {
  tlbhi_t saved = mips32_getasid();
  int i = _tlb_probe(e->hi);
  if (i >= 0)
    _tlb_write(i, e);
  else
    _tlb_write_random(e);
  mips32_setasid(saved);
}

int tlb_probe(tlbentry_t *e) {
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

void tlb_print(void) {
  tlbhi_t saved = mips32_getasid();
  kprintf("[tlb] TLB state [ASID=%ld]:\n", saved);
  for (unsigned i = 0; i < tlb_size(); i++) {
    tlbentry_t e;
    _tlb_read(i, &e);
    if ((e.lo0 & PTE_VALID) || (e.lo1 & PTE_VALID)) {
      kprintf("[tlb] %d => ASID: %02lx", i, e.hi & PTE_ASID_MASK);
      if (e.lo0 & PTE_VALID)
        kprintf(" PFN0: {%08lx => %08lx %c%c}", e.hi & PTE_VPN2_MASK,
                PTE_PFN_OF(e.lo0), (e.lo0 & PTE_DIRTY) ? 'D' : '-',
                (e.lo0 & PTE_GLOBAL) ? 'G' : '-');
      if (e.lo1 & PTE_VALID)
        kprintf(" PFN1: {%08lx => %08lx %c%c}",
                (e.hi & PTE_VPN2_MASK) + PAGESIZE, PTE_PFN_OF(e.lo1),
                (e.lo1 & PTE_DIRTY) ? 'D' : '-',
                (e.lo1 & PTE_GLOBAL) ? 'G' : '-');
      kprintf("\n");
    }
  }
  mips32_setasid(saved);
}

static tlbentry_t _gdb_tlb_entry;

/* Fills _dgb_tlb_entry structure with TLB entry. Used by debugger. */
void _gdb_tlb_read_index(unsigned i) {
  tlb_read(i, &_gdb_tlb_entry);
}
