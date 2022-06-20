#ifndef _AARCH64_PMAP_H_
#define _AARCH64_PMAP_H_

#include <stdbool.h>
#include <sys/klog.h>
#include <sys/vm.h>
#include <aarch64/pte.h>

/* Number of page directory entries. */
#define PD_ENTRIES (PAGESIZE / (int)sizeof(pde_t))

/* Number of page table entries. */
#define PT_ENTRIES (PAGESIZE / (int)sizeof(pte_t))

/* XXX Raspberry PI 3 specific! */
#define DMAP_SIZE 0x3c000000
#define DMAP_BASE 0xffffff8000000000 /* last 512GB */

#define DMAP_L3_ENTRIES max(1, DMAP_SIZE / PAGESIZE)
#define DMAP_L2_ENTRIES max(1, DMAP_L3_ENTRIES / PT_ENTRIES)
#define DMAP_L1_ENTRIES max(1, DMAP_L2_ENTRIES / PT_ENTRIES)

#define DMAP_L1_SIZE roundup(DMAP_L1_ENTRIES * sizeof(pde_t), PAGESIZE)
#define DMAP_L2_SIZE roundup(DMAP_L2_ENTRIES * sizeof(pde_t), PAGESIZE)
#define DMAP_L3_SIZE roundup(DMAP_L3_ENTRIES * sizeof(pte_t), PAGESIZE)

#define PA_MASK 0xfffffffff000
#define PTE_FRAME_ADDR(pte) ((pte)&PA_MASK)

#define PAGE_TABLE_DEPTH 4

#define PTE_EMPTY_KERNEL 0
#define PTE_EMPTY_USER 0

#define PTE_SET_ON_REFERENCED ATTR_AF
#define PTE_CLR_ON_REFERENCED 0

#define PTE_SET_ON_MODIFIED 0
#define PTE_CLR_ON_MODIFIED ATTR_AP_RO

#define GROWKERNEL_STRIDE L2_SIZE

typedef struct pmap pmap_t;

typedef struct pmap_md {
} pmap_md_t;

/*
 * Page directory.
 */

static inline bool pde_valid_p(pde_t *pdep) {
  return pdep && PTE_FRAME_ADDR(*pdep) != 0;
}

void *phys_to_dmap(paddr_t addr);

static inline pde_t *pde_ptr(paddr_t pd_pa, int lvl, vaddr_t va) {
  pde_t *pde = phys_to_dmap(pd_pa);
  if (lvl == 0)
    return pde + L0_INDEX(va);
  if (lvl == 1)
    return pde + L1_INDEX(va);
  if (lvl == 2)
    return pde + L2_INDEX(va);
  return pde + L3_INDEX(va);
}

/*
 * Page table.
 */

static inline paddr_t pte_frame(pte_t pte) {
  return PTE_FRAME_ADDR(pte);
}

static inline bool pte_valid_p(pte_t *ptep) {
  return ptep && (pte_frame(*ptep) != 0);
}

static inline bool pte_access(pte_t pte, vm_prot_t prot) {
  switch (prot) {
    case VM_PROT_READ:
      return pte & ATTR_SW_READ;
    case VM_PROT_WRITE:
      return pte & ATTR_SW_WRITE;
    case VM_PROT_EXEC:
      return !(pte & ATTR_SW_NOEXEC);
    default:
      panic("Invalid pte_access invocation (prot=%x)", prot);
  }
}

/*
 * Physical map management.
 */

static inline void pmap_md_setup(pmap_t *pmap) {
}

static inline void pmap_md_delete(pmap_t *pmap) {
}

static inline void pmap_md_growkernel(vaddr_t maxkvaddr) {
}

#endif /* !_AARCH64_PMAP_H_ */
