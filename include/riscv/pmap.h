#ifndef _RISCV_PMAP_H_
#define _RISCV_PMAP_H_

#include <stdbool.h>
#include <sys/klog.h>
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

#define PAGE_TABLE_DEPTH 2

#define PTE_EMPTY_KERNEL 0
#define PTE_EMPTY_USER 0

#define PTE_SET_ON_REFERENCED (PTE_A | PTE_V)
#define PTE_CLR_ON_REFERENCED 0

#define PTE_SET_ON_MODIFIED (PTE_D | PTE_W)
#define PTE_CLR_ON_MODIFIED 0

#define GROWKERNEL_STRIDE L0_SIZE

typedef struct pmap pmap_t;

typedef struct pmap_md {
  paddr_t satp;      /* supervisor address translation and protection */
  u_long generation; /* current generation number */
  LIST_ENTRY(pmap) pmap_link; /* link on `user_pmaps` */
} pmap_md_t;

/*
 * Page directory.
 */

static inline bool pde_valid_p(pde_t *pdep) {
  return pdep && VALID_PDE_P(*pdep);
}

void *phys_to_dmap(paddr_t addr);

static inline pde_t *pde_ptr(paddr_t pd_pa, int lvl, vaddr_t va) {
  pde_t *pde = phys_to_dmap(pd_pa);
  if (lvl == 0)
    return pde + L0_INDEX(va);
  return pde + L1_INDEX(va);
}

/*
 * Page table.
 */

static inline paddr_t pte_frame(pte_t pte) {
  return PTE_TO_PA(pte);
}

static inline bool pte_valid_p(pte_t *ptep) {
  return ptep && VALID_PTE_P(*ptep);
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

#endif /* !_RISCV_PMAP_H_ */
