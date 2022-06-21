#include <sys/klog.h>
#include <riscv/pmap.h>

/*
 * NOTE: the `sfence.vma` instruction has multiple variants
 * which perform the fence operation with different granularity.
 * Unfortunately, some cores doesn't support the more specialized
 * variants, e.g. an `sfence.vma` with arguments executed on
 * the VexRiscv softcore results in an illegal instruction exception.
 * Thereby, Mimiker relies on the generic flush without any arguments.
 */

void tlb_invalidate(vaddr_t va __unused, asid_t asid __unused) {
  __asm __volatile("sfence.vma" ::: "memory");
}

void tlb_invalidate_asid(asid_t asid __unused) {
  __asm __volatile("sfence.vma" ::: "memory");
}
