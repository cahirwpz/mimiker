#ifndef _AARCH64_PMAP_H_
#define _AARCH64_PMAP_H_

#include <stdbool.h>
#include <sys/klog.h>
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

#define PTE_SET_ON_REFERENCED ATTR_AF
#define PTE_CLR_ON_REFERENCED 0

#define PTE_SET_ON_MODIFIED 0
#define PTE_CLR_ON_MODIFIED ATTR_AP_RO

#define GROWKERNEL_STRIDE L2_SIZE

typedef struct pmap_md {
} pmap_md_t;

/*
 * Translation structure.
 */

static inline size_t pt_index(unsigned lvl, vaddr_t va) {
  assert(lvl < PAGE_TABLE_DEPTH);
  if (lvl == 0)
    return L0_INDEX(va);
  if (lvl == 1)
    return L1_INDEX(va);
  if (lvl == 2)
    return L2_INDEX(va);
  return L3_INDEX(va);
}

/*
 * Page directory.
 */

static inline bool pde_valid_p(pde_t pde) {
  return PTE_FRAME_ADDR(pde) != 0UL;
}

static inline paddr_t pde2pa(pde_t pde) {
  return PTE_FRAME_ADDR(pde);
}

/*
 * Page table.
 */

static inline bool pte_valid_p(pte_t pte) {
  return PTE_FRAME_ADDR(pte) != 0UL;
}

static inline bool pte_readable(pte_t pte) {
  return pte & ATTR_SW_READ;
}

static inline bool pte_writable(pte_t pte) {
  return pte & ATTR_SW_WRITE;
}

static inline bool pte_executable(pte_t pte) {
  return !(pte & ATTR_SW_NOEXEC);
}

static inline pte_t pte_empty(bool kernel) {
  return 0;
}

static inline paddr_t pte2pa(pte_t pte) {
  return PTE_FRAME_ADDR(pte);
}

/* MD pmap bootstrap. */
void pmap_bootstrap(paddr_t pd_pa);

#endif /* !_AARCH64_PMAP_H_ */
