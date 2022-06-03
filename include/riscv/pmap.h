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

/*
 * TODO: implement generic pmap interface.
 */

#define PAGE_TABLE_DEPTH 2

#define PTE_EMPTY_KERNEL 0
#define PTE_EMPTY_USER 0

#define PTE_SET_ON_REFERENCED 0
#define PTE_CLR_ON_REFERENCED 0

#define PTE_SET_ON_MODIFIED 0
#define PTE_CLR_ON_MODIFIED 0

#define GROWKERNEL_STRIDE L0_SIZE

typedef struct pmap pmap_t;

typedef struct pmap_md {
} pmap_md_t;

/*
 * Page directory.
 */

static inline bool pde_valid_p(pde_t *pdep) {
  panic("Not implemented!");
}

static inline pde_t *pde_ptr(paddr_t pd_pa, int lvl, vaddr_t va) {
  panic("Not implemented!");
}

/*
 * Page table.
 */

static inline paddr_t pte_frame(pte_t pte) {
  panic("Not implemented!");
}

static inline bool pte_valid_p(pte_t *ptep) {
  panic("Not implemented!");
}

static inline bool pte_access(pte_t pte, vm_prot_t prot) {
  panic("Not implemented!");
}

#endif /* !_RISCV_PMAP_H_ */
