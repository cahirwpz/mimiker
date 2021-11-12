#ifndef _RISCV_VM_PARAM_H_
#define _RSICV_VM_PARAM_H_

#define KERNEL_SPACE_BEGIN 0x80000000
#define KERNEL_SPACE_END 0xffffffff

#define USER_SPACE_BEGIN 0x00400000
#define USER_SPACE_END 0x80000000

#define USER_STACK_TOP 0x7f800000
#define USER_STACK_SIZE 0x800000 /* grows down up to that size limit */

#define PAGESIZE 4096

#define VM_PHYSSEG_NMAX 16

#define KSTACK_PAGES 1
#define KSTACK_SIZE (KSTACK_PAGES * PAGESIZE)

/* Size of boot memory allocation area. */
#define BOOTMEM_PAGES 2
#define BOOTMEM_SIZE (BOOTMEM_PAGES * PAGESIZE)

#define RISCV_PHYSADDR(x)                                                      \
  ((paddr_t)((vaddr_t)(x) & ~KERNEL_SPACE_BEGIN) + KERNEL_PHYS)

#define KERNEL_PHYS_END                                                        \
  (RISCV_PHYSADDR(align((paddr_t)__ebss, PAGESIZE)) + BOOTMEM_SIZE)

#endif /* !_RISCV_VM_PARAM_H_ */
