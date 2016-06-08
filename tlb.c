#include <tlb.h>
#include <mips/cpu.h>
#include <mips/m32c0.h>
#include <libkern.h>
#include <vm_phys.h>

#define PAGE_MASK_4KB 0 /* We need only 4KB page size masks at the moment */

void tlb_init() {
  tlb_invalidate_all();
  /* Shift C0_CONTEXT left, because we shift it right in tlb_refill_handler.
   * This is little hack to make page table sized 4MB, but causes us to
   * keep PTE in KSEG2. */
  mips32_set_c0(C0_CONTEXT, PTE_BASE << 1);
}

static uint8_t get_asid() {
  return mips32_get_c0(C0_ENTRYHI) & ASID_MASK;
}

static void set_asid(uint8_t asid) {
  mips32_set_c0(C0_ENTRYHI, asid);
}

void tlb_invalidate(tlbhi_t hi) {
  uint8_t asid = get_asid();
  mips_tlbinval(hi);
  set_asid(asid);
}

void tlb_invalidate_all() {
  mips_tlbinvalall();
}

void tlb_read_index(tlbhi_t *hi, tlblo_t *lo0, tlblo_t *lo1, int i) {
  uint8_t asid = get_asid();
  unsigned dummy;
  mips_tlbri2(hi, lo0, lo1, &dummy, i);
  set_asid(asid);
}

void tlb_write_index(tlbhi_t hi, tlblo_t lo0, tlblo_t lo1, int i) {
  uint8_t asid = get_asid();
  mips_tlbwi2(hi, lo0, lo1, PAGE_MASK_4KB, i);
  set_asid(asid);
}

void tlb_write_random(tlbhi_t hi, tlblo_t lo0, tlblo_t lo1) {
  uint8_t asid = get_asid();
  mips_tlbwr2(hi, lo0, lo1, PAGE_MASK_4KB);
  set_asid(asid);
}

void tlb_probe2(tlbhi_t hi, tlblo_t *lo0, tlblo_t *lo1) {
  uint8_t asid = get_asid();
  unsigned dummy;
  mips_tlbprobe2(hi, lo0, lo1, &dummy);
  set_asid(asid);
}

void tlb_overwrite_random(tlbhi_t hi, tlblo_t lo0, tlblo_t lo1) {
  uint8_t asid = get_asid();
  mips_tlbrwr2(hi, lo0, lo1, PAGE_MASK_4KB);
  set_asid(asid);
}

void tlb_print() {
  kprintf("[tlb] C0_CONTEXT     : %p\n", mips32_get_c0(C0_CONTEXT));
  kprintf("[tlb] C0_CONTEXT >> 1: %p\n", (int)mips32_get_c0(C0_CONTEXT) >> 1);
  kprintf("[tlb] ASID           : %d\n", get_asid());

  size_t n = mips_tlb_size();
  for (size_t i = 0; i < n; i++) {
    tlbhi_t entryhi;
    tlblo_t entrylo0;
    tlblo_t entrylo1;
    tlb_read_index(&entryhi, &entrylo0, &entrylo1, i);
    kprintf("[tlb: %d]: hi: %p lo0:%p lo1: %p \n", i, entryhi, entrylo0, entrylo1);
  }
}

