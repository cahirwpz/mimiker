#ifndef _RISCV_PMAP_H_
#define _RISCV_PMAP_H_

#include <riscv/pte.h>

/*
 * Direct map.
 */
#define DMAP_VADDR_BASE 0xc0000000
#define DMAP_MAX_SIZE 0x40000000

int pmap_bootstrap(pd_entry_t *pde, paddr_t min_pa, size_t pm_size);

#endif /* !_RISCV_PMAP_H_ */
