#ifndef _AARCH64_PMAP_H_
#define _AARCH64_PMAP_H_

#include <aarch64/pte.h>

typedef struct pmap pmap_t;

/* Number of page directory entries. */
#define PD_ENTRIES (PAGESIZE / (int)sizeof(pde_t))

/* Number of page table entries. */
#define PT_ENTRIES (PAGESIZE / (int)sizeof(pte_t))

#define PMAP_KERNEL_BEGIN 0xffff000000000000L
#define PMAP_KERNEL_END 0xffffffffffffffffL

#define PMAP_USER_BEGIN 0x0000000000400000L
#define PMAP_USER_END 0x0000800000000000L

/* XXX Raspberry PI 3 specific! */
#define DMAP_SIZE 0x3c000000
#define DMAP_BASE 0xffffff8000000000 /* last 512GB */

#define PHYS_TO_DMAP(x) ((uintptr_t)(x) + DMAP_BASE)

#define DMAP_L3_ENTRIES max(1, DMAP_SIZE / PAGESIZE)
#define DMAP_L2_ENTRIES max(1, DMAP_L3_ENTRIES / PT_ENTRIES)
#define DMAP_L1_ENTRIES max(1, DMAP_L2_ENTRIES / PT_ENTRIES)

#define DMAP_L1_SIZE roundup(DMAP_L1_ENTRIES * sizeof(pde_t), PAGESIZE)
#define DMAP_L2_SIZE roundup(DMAP_L2_ENTRIES * sizeof(pde_t), PAGESIZE)
#define DMAP_L3_SIZE roundup(DMAP_L3_ENTRIES * sizeof(pte_t), PAGESIZE)

#define PA_MASK 0xfffffffff000
#define PTE_FRAME_ADDR(pte) ((pte)&PA_MASK)

#endif /* !_AARCH64_PMAP_H_ */
