#ifndef _AARCH64_VM_PARAM_H_
#define _AARCH64_VM_PARAM_H_

#define PAGESIZE 4096
#define SUPERPAGESIZE (1 << 21) /* 2 MB */

#define KERNEL_SPACE_BEGIN 0xffff000000000000L
#define KERNEL_SPACE_END 0xffffffffffffffffL

#define USER_SPACE_BEGIN 0x0000000000400000L
#define USER_SPACE_END 0x0000800000000000L

#define USER_STACK_TOP 0x00007fffffff0000L
#define USER_STACK_SIZE 0x800000 /* grows down up to that size limit */

#define VM_PHYSSEG_NMAX 16

#define KSTACK_PAGES 2
#define KSTACK_SIZE (KSTACK_PAGES * PAGESIZE)

/* XXX Raspberry PI 3 specific! */
#define DMAP_SIZE 0x3c000000
#define DMAP_BASE 0xffffff8000000000 /* last 512GB */

#define PHYS_TO_DMAP(x) ((intptr_t)(x) + DMAP_BASE)

#define DMAP_L3_ENTRIES max(1, DMAP_SIZE / PAGESIZE)
#define DMAP_L2_ENTRIES max(1, DMAP_L3_ENTRIES / PT_ENTRIES)
#define DMAP_L1_ENTRIES max(1, DMAP_L2_ENTRIES / PT_ENTRIES)

#define DMAP_L1_SIZE roundup(DMAP_L1_ENTRIES * sizeof(pde_t), PAGESIZE)
#define DMAP_L2_SIZE roundup(DMAP_L2_ENTRIES * sizeof(pde_t), PAGESIZE)
#define DMAP_L3_SIZE roundup(DMAP_L3_ENTRIES * sizeof(pte_t), PAGESIZE)

#endif /* !_AARCH64_VM_PARAM_H_ */
