#ifndef _MIPS_PCPU_H_
#define _MIPS_PCPU_H_

#define PCPU_MD_FIELDS                                                         \
  struct {                                                                     \
    /*!< kernel sp restored on user->kernel transition */                      \
    void *ksp;                                                                 \
    /*!< registers that cannot be saved directly to kernel stack */            \
    register_t status, sp, cause, epc, badvaddr;                               \
  }

#ifdef _MACHDEP
#ifdef __ASSEMBLER__

#include <mips/asm.h>
#include <mips/mips.h>

#define LOAD_PCPU(reg) LA reg, _pcpu_data

#endif /* !__ASSEMBLER__ */
#endif /* !_MACHDEP */

#endif /* !_MIPS_PCPU_H_ */
