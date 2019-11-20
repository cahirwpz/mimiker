#ifndef _MIPS_VM_PARAM_H_
#define _MIPS_VM_PARAM_H_

#include <mips/mips.h>

#ifndef __ASSEMBLER__
extern char __kernel_start[];
extern char __kernel_end[];
#endif /* __ASSEMBLER__ */

#define VM_MAP_KERNEL_BEGIN ((vaddr_t)vm_kernel_end)
#define VM_MAP_KERNEL_END 0xffffe000

#define VM_MAP_USER_BEGIN 0x00400000
#define VM_MAP_USER_END 0x80000000

#define PAGESIZE 4096

#define VM_PHYSSEG_NMAX 16

#endif /* !_MIPS_VM_PARAM_H_ */
