#ifndef _AARCH64_PMAP_H_
#define _AARCH64_PMAP_H_

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

#define PMAP_MD_FIELDS                                                         \
  struct {}

#define PTE_SET_ON_REFERENCED ATTR_AF
#define PTE_CLR_ON_REFERENCED 0

#define PTE_SET_ON_MODIFIED 0
#define PTE_CLR_ON_MODIFIED ATTR_AP_RO

#define GROWKERNEL_STRIDE L2_SIZE

void pmap_bootstrap(paddr_t pd_pa);

#endif /* !_AARCH64_PMAP_H_ */
