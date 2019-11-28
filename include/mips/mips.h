#ifndef _MIPS_MIPS_H_
#define _MIPS_MIPS_H_

#include <mips/m32c0.h>

/*
 * Initial virtual address space is partitioned into four segments:
 *
 *   kuseg   0x00000000 - 0x7fffffff  User virtual memory, TLB mapped
 *   kseg0   0x80000000 - 0x9fffffff  Physical memory, cached, unmapped
 *   kseg1   0xa0000000 - 0xbfffffff  Physical memory, uncached, unmapped
 *   kseg2   0xc0000000 - 0xffffffff  Kernel virtual memory, TLB mapped
 */

#define MIPS_KSEG0_START 0x80000000
#define MIPS_KSEG1_START 0xa0000000
#define MIPS_KSEG2_START 0xc0000000
#define MIPS_PHYS_MASK 0x1fffffff

#ifndef __ASSEMBLER__

#define MIPS_KSEG0_TO_PHYS(x) (paddr_t)((uintptr_t)(x)&MIPS_PHYS_MASK)
#define MIPS_PHYS_TO_KSEG0(x) (vaddr_t)((uintptr_t)(x) | MIPS_KSEG0_START)
#define MIPS_KSEG1_TO_PHYS(x) (paddr_t)((uintptr_t)(x)&MIPS_PHYS_MASK)
#define MIPS_PHYS_TO_KSEG1(x) (vaddr_t)((uintptr_t)(x) | MIPS_KSEG1_START)
#define MIPS_KSEG2_TO_PHYS(x) (paddr_t)((uintptr_t)(x)&MIPS_PHYS_MASK)
#define MIPS_PHYS_TO_KSEG2(x) (vaddr_t)((uintptr_t)(x) | MIPS_KSEG2_START)

#define MIPS_KSEG2_TO_KSEG0(x) MIPS_PHYS_TO_KSEG0(MIPS_KSEG2_TO_PHYS(x))

extern char _ebase[];
extern char __boot[];
extern char __text[];
extern char __data[];
extern char __bss[];
extern char __ebss[];

#else /* __ASSEMBLER__ */

#define MIPS_KSEG2_TO_KSEG0(x) ((x) - (MIPS_KSEG2_START - MIPS_KSEG0_START))

#endif

#endif /* !_MIPS_MIPS_H */
