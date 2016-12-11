#include <stdc.h>
#include <mips/mips.h>
#include <mips/tlb.h>
#include <vm.h>

void tlb_init() {
  tlb_invalidate_all();
  /* Shift C0_CONTEXT left, because we shift it right in tlb_refill_handler.
   * This is little hack to make page table sized 4MB, but causes us to
   * keep PTE in KSEG2. */
  mips32_set_c0(C0_CONTEXT, PT_BASE << 1);
}

void tlb_print() {
  uint32_t n = tlb_size();
  for (uint32_t i = 0; i < n; i++) {
    tlbhi_t hi;
    tlblo_t lo0, lo1;
    tlb_read_index(&hi, &lo0, &lo1, i);
    if ((lo0 & PTE_VALID) || (lo1 & PTE_VALID)) {
      kprintf("[tlb] %ld => ASID: %02x", i, hi & PTE_ASID_MASK);
      if (lo0 & PTE_VALID)
        kprintf(" PFN0: {%08x => %08x %c%c}", hi & PTE_VPN2_MASK,
                PTE_PFN_OF(lo0), (lo0 & PTE_DIRTY) ? 'D' : '-',
                (lo0 & PTE_GLOBAL) ? 'G' : '-');
      if (lo1 & PTE_VALID)
        kprintf(" PFN1: {%08x => %08x %c%c}", (hi & PTE_VPN2_MASK) + PAGESIZE,
                PTE_PFN_OF(lo1), (lo1 & PTE_DIRTY) ? 'D' : '-',
                (lo1 & PTE_GLOBAL) ? 'G' : '-');
      kprintf("\n");
    }
  }
}

static struct { tlbhi_t hi; tlblo_t lo0; tlblo_t lo1; } _gdb_tlb_entry;

/* Fills _dgb_tlb_entry structure with TLB entry. Used by debugger. */
void _gdb_tlb_read_index(unsigned idx) {
  tlb_read_index(&_gdb_tlb_entry.hi, &_gdb_tlb_entry.lo0, &_gdb_tlb_entry.lo1, idx);
}
