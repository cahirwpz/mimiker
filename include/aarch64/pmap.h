#ifndef _AARCH64_PMAP_H_
#define _AARCH64_PMAP_H_

#include <aarch64/pte.h>

typedef struct pmap pmap_t;

/* Number of page directory entries. */
#define PD_ENTRIES (PAGESIZE / (int)sizeof(pde_t))

/* Number of page table entries. */
#define PT_ENTRIES (PAGESIZE / (int)sizeof(pte_t))

#define PMAP_KERNEL_BEGIN 0xffff000000000000L
#define PMAP_KERNEL_END 0xffffffffffffffffL

#define PMAP_USER_BEGIN 0x0000000000400000L
#define PMAP_USER_END 0x0000800000000000L

#endif /* !_AARCH64_PMAP_H_ */
