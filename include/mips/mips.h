#ifndef _MIPS_MIPS_H_
#define _MIPS_MIPS_H_

#ifndef _MACHDEP
#error "Do not use this header file outside kernel machine dependent code!"
#endif

/*
 * Initial virtual address space is partitioned into four segments:
 *
 *   kuseg   0x00000000 - 0x7fffffff  User virtual memory, TLB mapped
 *   kseg0   0x80000000 - 0x9fffffff  Physical memory, cached, unmapped
 *   kseg1   0xa0000000 - 0xbfffffff  Physical memory, uncached, unmapped
 *   kseg2   0xc0000000 - 0xdfffffff  Kernel virtual memory, TLB mapped
 *   kseg3   0xe0000000 - 0xffffffff  Kernel virtual memory, TLB mapped
 */

#define MIPS_KSEG0_START 0x80000000
#define MIPS_KSEG1_START 0xa0000000
#define MIPS_KSEG2_START 0xc0000000
#define MIPS_KSEG3_START 0xe0000000
#define MIPS_PHYS_MASK 0x1fffffff

#ifndef __ASSEMBLER__

#define MIPS_KSEG0_TO_PHYS(x) (paddr_t)((uintptr_t)(x)&MIPS_PHYS_MASK)
#define MIPS_PHYS_TO_KSEG0(x) (vaddr_t)((uintptr_t)(x) | MIPS_KSEG0_START)
#define MIPS_KSEG1_TO_PHYS(x) (paddr_t)((uintptr_t)(x)&MIPS_PHYS_MASK)
#define MIPS_PHYS_TO_KSEG1(x) (vaddr_t)((uintptr_t)(x) | MIPS_KSEG1_START)
#define MIPS_KSEG2_TO_PHYS(x) (paddr_t)((uintptr_t)(x)&MIPS_PHYS_MASK)
#define MIPS_PHYS_TO_KSEG2(x) (vaddr_t)((uintptr_t)(x) | MIPS_KSEG2_START)

#define MIPS_KSEG2_TO_KSEG0(x) MIPS_PHYS_TO_KSEG0(MIPS_KSEG2_TO_PHYS(x))

#define MIPS_IN_KSEG2_P(x)                                                     \
  (((uintptr_t)(x) >= MIPS_KSEG2_START) && ((uintptr_t)(x) < MIPS_KSEG3_START))

/* Symbols provided by the linker. */
extern char _gp[];
/* Exception vector base. */
extern char _ebase[];

const char *const exceptions[32];

#else /* __ASSEMBLER__ */

#define MIPS_KSEG2_TO_KSEG0(x) ((x) - (MIPS_KSEG2_START - MIPS_KSEG0_START))

#endif

#endif /* !_MIPS_MIPS_H */
