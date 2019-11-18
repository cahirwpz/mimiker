#ifndef _MIPS_VM_PARAM_H_
#define _MIPS_VM_PARAM_H_

#include <mips/mips.h>

#ifndef __ASSEMBLER__
extern char __kernel_start[];
extern char *__kernel_end;
#endif /* __ASSEMBLER__ */

#define VM_MAP_KERNEL_BEGIN MIPS_PHYS_TO_KSEG2(MIPS_KSEG0_TO_PHYS(__kernel_end))
#define VM_MAP_KERNEL_END 0xffffe000

#define VM_MAP_USER_BEGIN 0x00400000
#define VM_MAP_USER_END 0x80000000

#define PAGESIZE 4096

#endif /* !_MIPS_VM_PARAM_H_ */
