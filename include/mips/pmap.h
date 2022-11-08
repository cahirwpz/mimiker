#ifndef _MIPS_PMAP_H_
#define _MIPS_PMAP_H_

#ifndef _MACHDEP
#error "Do not use this header file outside kernel machine dependent code!"
#endif

#ifndef __ASSEMBLER__
#include <sys/types.h>
#include <mips/mips.h>
#include <mips/tlb.h>

typedef uint8_t asid_t;
typedef uint32_t pte_t;
typedef uint32_t pde_t;

#endif /* __ASSEMBLER__ */

#include <mips/vm_param.h>

/* MIPS pmap implements standard two-level hierarchical page table
 * stored in physical addresses. Indices are 10-bits wide. */
#define PTE_INDEX_MASK 0x003ff000
#define PTE_INDEX_SHIFT 12
#define PDE_INDEX_MASK 0xffc00000
#define PDE_INDEX_SHIFT 22

#define PTE_INDEX(x) ((((vaddr_t)(x)) & PTE_INDEX_MASK) >> PTE_INDEX_SHIFT)
#define PDE_INDEX(x) ((((vaddr_t)(x)) & PDE_INDEX_MASK) >> PDE_INDEX_SHIFT)

#ifndef __ASSEMBLER__
/* Number of page directory entries. */
#define PD_ENTRIES (PAGESIZE / (int)sizeof(pde_t))

static_assert(PD_ENTRIES == 1 << 10,
              "Page directory describes 10-bit range addresses!");

/* Number of page table entries. */
#define PT_ENTRIES (PAGESIZE / (int)sizeof(pte_t))

static_assert(PT_ENTRIES == 1 << 10,
              "Page table describes 10-bit range addresses!");
#endif /* __ASSEMBLER__ */

/* Base addresses of active user and kernel page directory tables.
 * UPD_BASE must begin at 8KiB boundary. */
#define UPD_BASE (KERNEL_SPACE_END + PAGESIZE * 0)
#define KPD_BASE (KERNEL_SPACE_END + PAGESIZE * 1)

#ifndef __ASSEMBLER__

#include <stdbool.h>
#include <sys/klog.h>
#include <sys/vm.h>

#define PAGE_TABLE_DEPTH 2

#define DMAP_BASE MIPS_KSEG0_START

#define PTE_EMPTY_KERNEL PTE_GLOBAL
#define PTE_EMPTY_USER 0

#define PTE_SET_ON_REFERENCED PTE_VALID
#define PTE_CLR_ON_REFERENCED 0

#define PTE_SET_ON_MODIFIED PTE_DIRTY
#define PTE_CLR_ON_MODIFIED 0

#define GROWKERNEL_STRIDE (PAGESIZE * PAGESIZE / sizeof(pte_t))

typedef struct pmap pmap_t;

typedef struct pmap_md {
} pmap_md_t;

/*
 * Page directory.
 */

static inline bool pde_valid_p(pde_t *pdep) {
  return pdep && (*pdep & PDE_VALID);
}

void *phys_to_dmap(paddr_t addr);

static inline pde_t *pde_ptr(paddr_t pd_pa, int lvl, vaddr_t va) {
  pde_t *pde = phys_to_dmap(pd_pa);
  if (lvl == 0)
    return pde + PDE_INDEX(va);
  return pde + PTE_INDEX(va);
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
      return pte & PTE_SW_READ;
    case VM_PROT_WRITE:
      return pte & PTE_SW_WRITE;
    case VM_PROT_EXEC:
      return !(pte & PTE_SW_NOEXEC);
    default:
      panic("Invalid pte_access invocation (prot=%x)", prot);
  }
}

/*
 * Physical map management.
 */

static inline void pmap_md_delete(pmap_t *pmap) {
}

static inline void pmap_md_update(pmap_t *pmap) {
}

#endif /* __ASSEMBLER__ */

#endif /* !_MIPS_PMAP_H_ */
