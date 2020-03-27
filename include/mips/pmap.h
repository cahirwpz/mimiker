#ifndef _MIPS_PMAP_H_
#define _MIPS_PMAP_H_

#ifdef _MACHDEP

/* MIPS pmap implements standard two-level hierarchical page table
 * stored in physical addresses. Indices are 10-bits wide. */
#define PTE_INDEX_MASK 0x003ff000
#define PTE_INDEX_SHIFT 12
#define PTE_SIZE 4
#define PDE_INDEX_MASK 0xffc00000
#define PDE_INDEX_SHIFT 22

#define PTE_INDEX(x) (((x)&PTE_INDEX_MASK) >> PTE_INDEX_SHIFT)
#define PDE_INDEX(x) (((x)&PDE_INDEX_MASK) >> PDE_INDEX_SHIFT)

#define PD_ENTRIES 1024 /* page directory entries */
#define PD_SIZE (PD_ENTRIES * PTE_SIZE)
#define PT_ENTRIES 1024 /* page table entries */
#define PT_SIZE (PT_ENTRIES * PTE_SIZE)

/* Base addresses of active user and kernel page directory tables.
 * UPD_BASE must begin at 8KiB boundary. */
#define UPD_BASE (PMAP_KERNEL_END + PD_SIZE * 0)
#define KPD_BASE (PMAP_KERNEL_END + PD_SIZE * 1)

#endif /* !_MACHDEP */

#ifndef __ASSEMBLER__
#include <sys/types.h>

typedef uint8_t asid_t;
typedef uint32_t pte_t;
typedef uint32_t pde_t;
#endif /* __ASSEMBLER__ */

#define PMAP_KERNEL_BEGIN 0xc0000000 /* kseg2 & kseg3 */
#define PMAP_KERNEL_END 0xffffe000
#define PMAP_USER_BEGIN 0x00001000
#define PMAP_USER_END 0x80000000

#endif /* !_MIPS_PMAP_H_ */
