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

static __no_profile inline bool pde_valid_p(pde_t *pdep) {
  return pdep && PTE_FRAME_ADDR(*pdep) != 0;
}

void *phys_to_dmap(paddr_t addr);

static __no_profile inline size_t pde_index(int lvl, vaddr_t va) {
  switch (lvl) {
    case 0:
      return L0_INDEX(va);
    case 1:
      return L1_INDEX(va);
    case 2:
      return L2_INDEX(va);
    case 3:
      return L3_INDEX(va);
    default:
      panic("Invalid level: %d", lvl);
  }
}

static __no_profile inline bool pde_valid_index(int lvl, size_t index) {
  switch (lvl) {
    case 0:
      return index < L0_ENTRIES;
    case 1:
    case 2:
    case 3:
      return index < Ln_ENTRIES;
    default:
      panic("Invalid level: %d", lvl);
  }
}

static __no_profile inline size_t pde_size(int lvl) {
  switch (lvl) {
    case 0:
      return L0_SIZE;
    case 1:
      return L1_SIZE;
    case 2:
      return L2_SIZE;
    case 3:
      return L3_SIZE;
    default:
      panic("Invalid level: %d", lvl);
  }
}

/*
 * Page table.
 */

static __no_profile inline paddr_t pte_frame(pte_t pte) {
  return PTE_FRAME_ADDR(pte);
}

static __no_profile inline bool pte_valid_p(pte_t *ptep) {
  return ptep && (pte_frame(*ptep) != 0);
}

static __no_profile inline bool pte_access(pte_t pte, vm_prot_t prot) {
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

static __no_profile inline void pmap_md_setup(pmap_t *pmap) {
}

static __no_profile inline void pmap_md_delete(pmap_t *pmap) {
}

static __no_profile inline void pmap_md_growkernel(vaddr_t old_kva,
                                                   vaddr_t new_kva) {
}

static __no_profile inline void pmap_md_update(pmap_t *pmap) {
}

#endif /* !_AARCH64_PMAP_H_ */
