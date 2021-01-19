#include <aarch64/tlb.h>

#define ASID_TO_PTE(x) ((uint64_t)(x) << ASID_SHIFT)

#define __tlbi(x, r) __asm__ volatile("TLBI " x ", %0" : : "r"(r))
#define __dsb(x) __asm__ volatile("DSB " x)
#define __isb() __asm__ volatile("ISB")

void tlb_invalidate(vaddr_t va, asid_t asid) {
  __dsb("ishst");

  if (asid > 0) {
    __tlbi("vae1is", ASID_TO_PTE(asid) | (va >> PAGE_SHIFT));
  } else {
    __tlbi("vaae1is", va >> PAGE_SHIFT);
  }

  __dsb("ish");
  __isb();
}

void tlb_invalidate_asid(asid_t asid) {
  __dsb("ishst");
  __tlbi("aside1is", ASID_TO_PTE(asid));
  __dsb("ish");
  __isb();
}
