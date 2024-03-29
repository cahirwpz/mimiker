#ifndef _RISCV_VM_PARAM_H_
#define _RISCV_VM_PARAM_H_

#if __riscv_xlen == 64
#define KERNEL_SPACE_BEGIN 0xffffffc000000000L
#define KERNEL_SPACE_END 0xffffffffffffffffL

#define USER_SPACE_BEGIN 0x0000000000400000L
#define USER_SPACE_END 0x0000004000000000L

#define USER_STACK_TOP 0x0000003fffff0000L
#define USER_STACK_SIZE 0x800000 /* grows down up to that size limit */

/* KASAN shadow map */
#define KASAN_SHADOW_START 0xffffffd800000000L
#define KASAN_MAX_SHADOW_SIZE (0x8L << 30) /* 8 GB */

#define KSTACK_PAGES 2

#else /* __riscv_xlen == 32 */

#define KERNEL_SPACE_BEGIN 0x80000000
#define KERNEL_SPACE_END 0xffffffff

#define USER_SPACE_BEGIN 0x00400000
#define USER_SPACE_END 0x80000000

#define USER_STACK_TOP 0x7f800000
#define USER_STACK_SIZE 0x800000 /* grows down up to that size limit */

/* KASAN shadow map */
#define KASAN_SHADOW_START 0xa0000000
#define KASAN_MAX_SHADOW_SIZE (1 << 26) /* 64 MB */

#define KSTACK_PAGES 1
#endif

#define PAGESIZE 4096

#define VM_PHYSSEG_NMAX 16

#define KSTACK_SIZE (KSTACK_PAGES * PAGESIZE)

#endif /* !_RISCV_VM_PARAM_H_ */
