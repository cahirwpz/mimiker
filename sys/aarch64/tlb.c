#include <aarch64/tlb.h>

#define PAGE_SHIFT 12
#define ASID_SHIFT 48
#define ASID_TO_PTE(x) ((uint64_t)(x) << ASID_SHIFT)

#define __tlbi(x, r) __asm__ volatile("TLBI " x ", %0" : : "r"(r))
#define __dsb(x) __asm__ volatile("DSB " x)
#define __isb() __asm__ volatile("ISB")

void tlb_invalidate(pte_t pte, asid_t asid) {
  __dsb("ishst");

  if (asid > 0) {
    __tlbi("vae1is", ASID_TO_PTE(asid) | (pte >> PAGE_SHIFT));
  } else {
    __tlbi("vaae1is", pte >> PAGE_SHIFT);
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
