#include <riscv/tlb.h>

void tlb_invalidate(vaddr_t va __unused, asid_t asid __unused) {
  __asm __volatile("sfence.vma" ::: "memory");
}

void tlb_invalidate_asid(asid_t asid __unused) {
  __asm __volatile("sfence.vma" ::: "memory");
}
