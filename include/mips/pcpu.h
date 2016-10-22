#ifndef _MIPS_PCPU_H_
#define _MIPS_PCPU_H_

#ifdef __ASSEMBLER__

#include <mips/asm.h>
#include <mips/mips.h>

#define GET_CPU_PCPU(reg)        LA reg, _pcpu_data
#define PCPU_OFFSET(reg, member) PCPU_##member(reg)

#endif // !__ASSEMBLER__
#endif // _MIPS_PCPU_H_
