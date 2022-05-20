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
#define DMAP_BASE 0xc0000000
#define DMAP_MAX_SIZE 0x40000000

#define RISCV_PHYSADDR(x)                                                      \
  ((paddr_t)((vaddr_t)(x) & ~KERNEL_SPACE_BEGIN) + KERNEL_PHYS)

/*
 * TODO: implement generic pmap interface.
 */

#define PAGE_TABLE_DEPTH 2
#define SINGLE_PD 1

#define PTE_SET_ON_REFERENCED 0
#define PTE_CLR_ON_REFERENCED 0

#define PTE_SET_ON_MODIFIED 0
#define PTE_CLR_ON_MODIFIED 0

#define GROWKERNEL_STRIDE 0

typedef struct pmap_md {
} pmap_md_t;

/*
 * Translation structure.
 */

static inline size_t pt_index(unsigned lvl, vaddr_t va) {
  panic("Not implemented!");
}

/*
 * Page directory.
 */

static inline bool pde_valid_p(pde_t pde) {
  panic("Not implemented!");
}

/*
 * Page table.
 */

static inline bool pte_valid_p(pte_t pte) {
  panic("Not implemented!");
}

static inline bool pte_access(pte_t pte, vm_prot_t prot) {
  panic("Not implemented!");
}

static inline pte_t pte_empty(bool kernel) {
  panic("Not implemented!");
}

static inline paddr_t pte_frame(pte_t pte) {
  panic("Not implemented!");
}

/* MD pmap bootstrap. */
void pmap_bootstrap(paddr_t pd_pa, vaddr_t pd_va);

#endif /* !_RISCV_PMAP_H_ */
