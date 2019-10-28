#ifndef _MIPS_PCPU_H_
#define _MIPS_PCPU_H_

#ifdef __ASSEMBLER__

#include <mips/asm.h>
#include <mips/mips.h>

#define LOAD_PCPU(reg) LA reg, _pcpu_data
#define LOAD_PCPU_KSEG0(reg) LA reg, MIPS_KSEG2_TO_KSEG0(_pcpu_data)

#endif /* !__ASSEMBLER__ */
#endif /* !_MIPS_PCPU_H_ */
