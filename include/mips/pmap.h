#ifndef __MIPS_PMAP_H__
#define __MIPS_PMAP_H__

#ifndef __ASSEMBLER__
#include <pmap.h>
#endif

#include <mips/mips.h>

#define PTE_MASK 0xfffff000
#define PTE_SHIFT 12
#define PDE_MASK 0xffc00000
#define PDE_SHIFT 22

#define PTE_SIZE 4

#define PTE_INDEX(x) (((x)&PTE_MASK) >> PTE_SHIFT)
#define PDE_INDEX(x) (((x)&PDE_MASK) >> PDE_SHIFT)

#define PTE_OF(pmap, addr) ((pmap)->pte[PTE_INDEX(addr)])
#define PDE_OF(pmap, addr) ((pmap)->pde[PDE_INDEX(addr)])
#define PTF_ADDR_OF(vaddr) (PT_BASE + PDE_INDEX(vaddr) * PTF_SIZE)

#define PD_ENTRIES 1024 /* page directory entries */
#define PD_SIZE (PD_ENTRIES * PTE_SIZE)
#define PTF_ENTRIES 1024 /* page table fragment entries */
#define PTF_SIZE (PTF_ENTRIES * PTE_SIZE)
#define PT_ENTRIES (PD_ENTRIES * PTF_ENTRIES)
#define PT_SIZE (PT_ENTRIES * PTE_SIZE)

#define PMAP_KERNEL_BEGIN MIPS_KSEG2_START
#define PMAP_KERNEL_END 0xfffff000 /* kseg2 & kseg3 */
#define PMAP_USER_BEGIN 0x00000000
#define PMAP_USER_END MIPS_KSEG0_START /* useg */

#define PT_BASE MIPS_KSEG2_START
#define PD_BASE (PT_BASE + PT_SIZE)

#endif /* !__MIPS_PMAP_H__ */
