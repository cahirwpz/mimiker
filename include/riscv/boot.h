#ifndef _RISCV_BOOT_H_
#define _RISCV_BOOT_H_

#include <sys/types.h>

#define PHYSADDR(x) ((paddr_t)((vaddr_t)(x) + (KERNEL_PHYS - KERNEL_VIRT)))
#define VIRTADDR(x) ((vaddr_t)((paddr_t)(x) + (KERNEL_VIRT - KERNEL_PHYS)))

#endif /* !_RISCV_BOOT_H_ */
