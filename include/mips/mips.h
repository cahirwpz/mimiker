#ifndef _MIPS_MIPS_H_
#define _MIPS_MIPS_H_

#include <mips/m32c0.h>

/* MIPS32r2 extract bits */
#define _mips32r2_ext(x, pos, sz)                                              \
  __extension__({                                                              \
    register_t __x = (x), __v;                                                 \
    __asm__("ext %0,%z1,%2,%3" : "=d"(__v) : "dJ"(__x), "I"(pos), "I"(sz));    \
    __v;                                                                       \
  })

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

#define MIPS_KSEG0_TO_PHYS(x) ((uintptr_t)(x)&MIPS_PHYS_MASK)
#define MIPS_PHYS_TO_KSEG0(x) ((uintptr_t)(x) | (intptr_t)MIPS_KSEG0_START)
#define MIPS_KSEG1_TO_PHYS(x) ((uintptr_t)(x)&MIPS_PHYS_MASK)
#define MIPS_PHYS_TO_KSEG1(x) ((uintptr_t)(x) | (intptr_t)MIPS_KSEG1_START)

#define MIPS_KSEG0_P(x) (((intptr_t)(x) & ~MIPS_PHYS_MASK) == MIPS_KSEG0_START)
#define MIPS_KSEG1_P(x) (((intptr_t)(x) & ~MIPS_PHYS_MASK) == MIPS_KSEG1_START)
#define MIPS_KSEG2_P(x) ((uintptr_t)MIPS_KSEG2_START <= (uintptr_t)(x))

const char *const exceptions[32];

#endif /* __ASSEMBLER__ */

#endif /* !_MIPS_MIPS_H */
