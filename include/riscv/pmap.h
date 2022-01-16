#ifndef _RISCV_PMAP_H_
#define _RISCV_PMAP_H_

#include <riscv/pte.h>

/*
 * Direct map.
 */
#define DMAP_VADDR_BASE 0xc0000000
#define DMAP_MAX_SIZE 0x40000000

void pmap_bootstrap(paddr_t pd_pa, vaddr_t pd_va);

#endif /* !_RISCV_PMAP_H_ */
