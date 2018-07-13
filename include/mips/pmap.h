#ifndef _MIPS_PMAP_H_
#define _MIPS_PMAP_H_

#define PTE_INDEX_MASK 0xfffff000
#define PTE_INDEX_SHIFT 12
#define PTE_SIZE 4
#define PDE_INDEX_MASK 0xffc00000
#define PDE_INDEX_SHIFT 22

#define PD_ENTRIES 1024 /* page directory entries */
#define PD_SIZE (PD_ENTRIES * PTE_SIZE)
#define PTF_ENTRIES 1024 /* page table fragment entries */
#define PTF_SIZE (PTF_ENTRIES * PTE_SIZE)
#define PT_ENTRIES (PD_ENTRIES * PTF_ENTRIES)
#define PT_SIZE (PT_ENTRIES * PTE_SIZE)
#define PT_SIZE_BITS 22

#define PMAP_KERNEL_BEGIN (MIPS_KSEG2_START + PT_SIZE + PD_SIZE * 2)
#define PMAP_KERNEL_END 0xfffff000 /* kseg2 & kseg3 */
#define PMAP_USER_BEGIN 0x00400000
#define PMAP_USER_END 0x80000000

#define PT_BASE MIPS_KSEG2_START
#define UPD_BASE (PT_BASE + PT_SIZE + PD_SIZE * 0)
#define KPD_BASE (PT_BASE + PT_SIZE + PD_SIZE * 1)

#endif /* !_MIPS_PMAP_H_ */
