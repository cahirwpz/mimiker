#include <stdc.h>
#include <mips/mips.h>
#include <mips/tlb.h>

void tlb_init() {
  tlb_invalidate_all();
  /* Shift C0_CONTEXT left, because we shift it right in tlb_refill_handler.
   * This is little hack to make page table sized 4MB, but causes us to
   * keep PTE in KSEG2. */
  mips32_set_c0(C0_CONTEXT, PTE_BASE << 1);
}

void tlb_print() {
  uint32_t n = tlb_size();
  for (uint32_t i = 0; i < n; i++) {
    tlbhi_t hi;
    tlblo_t lo0, lo1;
    tlb_read_index(&hi, &lo0, &lo1, i);
    if ((lo0 & PTE_VALID) || (lo1 & PTE_VALID)) {
      kprintf("[tlb] %ld => {VPN: %08x ASID: %02x",
              i, hi & PTE_VPN2_MASK, hi & PTE_ASID_MASK);
      if (lo0 & PTE_VALID)
        kprintf(" PFN0: %08x %c%c", PTE_PFN_OF(lo0),
                (lo0 & PTE_DIRTY) ? 'D' : '-',
                (lo0 & PTE_GLOBAL) ? 'G' : '-');
      if (lo1 & PTE_VALID)
        kprintf(" PFN1: %08x %c%c", PTE_PFN_OF(lo1),
                (lo1 & PTE_DIRTY) ? 'D' : '-',
                (lo1 & PTE_GLOBAL) ? 'G' : '-');
      kprintf("}\n");
    }
  }
}
