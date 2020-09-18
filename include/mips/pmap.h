#ifndef _MIPS_PMAP_H_
#define _MIPS_PMAP_H_

#ifndef _MACHDEP
#error "Do not use this header file outside kernel machine dependent code!"
#endif

#ifndef __ASSEMBLER__
#include <sys/types.h>

typedef uint8_t asid_t;
typedef uint32_t pte_t;
typedef uint32_t pde_t;
#endif /* __ASSEMBLER__ */

#include <mips/vm_param.h>

#define PT_BASE ((pte_t *)0xffc00000)

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
#define UPD_BASE (PMAP_KERNEL_END + PAGESIZE * 0)
#define KPD_BASE (PMAP_KERNEL_END + PAGESIZE * 1)

#define PMAP_KERNEL_BEGIN 0xc0000000 /* kseg2 & kseg3 */
#define PMAP_KERNEL_END 0xffffe000
#define PMAP_USER_BEGIN 0x00001000
#define PMAP_USER_END 0x80000000

#endif /* !_MIPS_PMAP_H_ */
