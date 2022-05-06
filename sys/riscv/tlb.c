#include <sys/klog.h>
#include <riscv/pmap.h>

void tlb_invalidate(vaddr_t va, asid_t asid) {
  panic("Not implemented!");
}

void tlb_invalidate_asid(asid_t asid) {
  panic("Not implemented!");
}
