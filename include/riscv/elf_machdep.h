#ifndef _RISCV_ELF_MACHDEP_H_
#define _RISCV_ELF_MACHDEP_H_

#ifdef _LP64
#define ELFSIZE 64
#else
#define ELFSIZE 32
#endif /* !_LP64 */

#define EM_ARCH EM_RISCV

#endif /* _RISCV_ELF_MACHDEP_H_ */
