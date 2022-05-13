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

#define PAGE_TABLE_DEPTH 2

#ifndef __ASSEMBLER__

#include <stdbool.h>

#define PMAP_MD_FIELDS                                                         \
  struct {}

#define DMAP_BASE MIPS_KSEG0_START

#define PTE_SET_ON_REFERENCED PTE_VALID
#define PTE_CLR_ON_REFERENCED 0

#define PTE_SET_ON_MODIFIED PTE_DIRTY
#define PTE_CLR_ON_MODIFIED 0

#define GROWKERNEL_STRIDE (PAGESIZE * PAGESIZE / sizeof(pte_t))

/*
 * Page directory.
 */

static inline bool pde_valid_p(pde_t pde) {
  return pde & PDE_VALID;
}

static inline paddr_t pde2pa(pde_t pde) {
  return PTE_FRAME_ADDR(pde);
}

/*
 * Page table.
 */

static inline bool pte_valid_p(pte_t pte) {
  return PTE_FRAME_ADDR(pte) != 0;
}

static inline bool pte_readable(pte_t pte) {
  return pte & PTE_SW_READ;
}

static inline bool pte_writable(pte_t pte) {
  return pte & PTE_SW_WRITE;
}

static inline bool pte_executable(pte_t pte) {
  return !(pte & PTE_SW_NOEXEC);
}

static inline paddr_t pte2pa(pte_t pte) {
  return PTE_FRAME_ADDR(pte);
}

#endif /* __ASSEMBLER__ */

#endif /* !_MIPS_PMAP_H_ */
