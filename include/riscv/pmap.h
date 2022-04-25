#ifndef _RISCV_PMAP_H_
#define _RISCV_PMAP_H_

/*
 * Direct map.
 */
#define DMAP_VADDR_BASE 0xc0000000
#define DMAP_MAX_SIZE 0x40000000

#define RISCV_PHYSADDR(x)                                                      \
  ((paddr_t)((vaddr_t)(x) & ~KERNEL_SPACE_BEGIN) + KERNEL_PHYS)

void pmap_bootstrap(paddr_t pd_pa, vaddr_t pd_va);

#endif /* !_RISCV_PMAP_H_ */
