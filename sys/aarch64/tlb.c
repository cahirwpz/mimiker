#include <aarch64/tlb.h>

#define ASID_TO_PTE(x) ((uint64_t)(x) << ASID_SHIFT)

#define __tlbi(x, r) __asm__ volatile("TLBI " x ", %0" : : "r"(r))
#define __dsb(x) __asm__ volatile("DSB " x)
#define __isb() __asm__ volatile("ISB")

void tlb_invalidate(vaddr_t va, asid_t asid) {
  __dsb("ishst");

  /*
   * vae1is - Invalidate translation used at EL1 for the specified VA and
   * Address Space Identifier (ASID) and the current VMID, Inner Shareable.
   * vaae1is - Invalidate all translations used at EL1 for the specified
   * address and current VMID and for all ASID values, Inner Shareable.
   *
   * Based on arm documentation
   * [https://developer.arm.com/documentation/ddi0488/h/system-control/aarch64-register-summary/aarch64-tlb-maintenance-operations]
   * and pmap_invalidate_page from FreeBSD - /sys/arm64/arm64/pmap.c.
   */
  if (asid > 0) {
    __tlbi("vae1is", ASID_TO_PTE(asid) | (va >> PAGE_SHIFT));
  } else {
    /*
     * We don't know why but NetBSD, FreeBSD and Linux use vaae1is for kernel.
     * Using vae1is for kernel still works for us but we don't want to leave
     * potential bug in that part of code.
     */
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
