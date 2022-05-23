#ifndef _RISCV_PMAP_H_
#define _RISCV_PMAP_H_

#include <stdbool.h>
#include <sys/klog.h>
#include <sys/pmap.h>
#include <sys/vm.h>
#include <riscv/pte.h>
#include <riscv/riscvreg.h>

/*
 * Direct map.
 */
#if __riscv_xlen == 64
#define DMAP_BASE 0xffffffe000000000L
#define DMAP_MAX_SIZE 0x2000000000L
#else
#define DMAP_BASE 0xc0000000
#define DMAP_MAX_SIZE 0x40000000
#endif

#define RISCV_PHYSADDR(x)                                                      \
  ((paddr_t)((vaddr_t)(x) & ~KERNEL_SPACE_BEGIN) + KERNEL_PHYS)

#if __riscv_xlen == 64
#define PAGE_TABLE_DEPTH 3
#define SHARED_KERNEL_PD 1
#define GROWKERNEL_STRIDE L1_SIZE
#else
#define PAGE_TABLE_DEPTH 2
#define SHARED_KERNEL_PD 1
#define GROWKERNEL_STRIDE L0_SIZE
#endif

#define PTE_SET_ON_REFERENCED (PTE_A | PTE_V)
#define PTE_CLR_ON_REFERENCED 0

#define PTE_SET_ON_MODIFIED (PTE_D | PTE_W)
#define PTE_CLR_ON_MODIFIED 0

typedef struct pmap_md {
  paddr_t satp; /* supervisor address translation and protection */
} pmap_md_t;

/*
 * Translation structure.
 */

static inline size_t pt_index(unsigned lvl, vaddr_t va) {
  if (lvl == 0)
    return L0_INDEX(va);
#if __riscv_xlen == 32
  return L1_INDEX(va);
#else
  if (lvl == 1)
    return L1_INDEX(va);
  return L2_INDEX(va);
#endif
}

/*
 * Page directory.
 */

static inline bool pde_valid_p(pde_t pde) {
  return pde & PTE_V;
}

/*
 * Page table.
 */

static inline bool pte_valid_p(pte_t pte) {
  return pte != 0;
}

static inline bool pte_access(pte_t pte, vm_prot_t prot) {
  switch (prot) {
    case VM_PROT_READ:
      return pte & PTE_SW_READ;
    case VM_PROT_WRITE:
      return pte & PTE_SW_WRITE;
    case VM_PROT_EXEC:
      return pte & PTE_X;
    default:
      panic("Invalid pte_access invocation (prot=%x)", prot);
  }
}

static inline pte_t pte_empty(bool kernel) {
  return 0;
}

static inline paddr_t pte_frame(pte_t pte) {
  return PTE_TO_PA(pte);
}

/* MD pmap bootstrap. */
void pmap_bootstrap(paddr_t pd_pa, vaddr_t pd_va);

#endif /* !_RISCV_PMAP_H_ */
