#ifndef _RISCV_SYSCALL_H_
#define _RISCV_SYSCALL_H_

#ifdef __ASSEMBLER__

#include <riscv/asm.h>

/* Do a syscall that is missing an implementation. */
#define SYSCALL_MISSING(name)                                                  \
  ENTRY(name);                                                                 \
  /* Not implemented! */                                                       \
  j _C_LABEL(name);                                                            \
  END(name)

/* Do a syscall that cannot fail. */
#define SYSCALL_NOERROR(name, num)                                             \
  ENTRY(name);                                                                 \
  /* Not implemented! */                                                       \
  j _C_LABEL(name);                                                            \
  END(name)

/* Do a normal syscall. */
#define SYSCALL(name, num)                                                     \
  ENTRY(name);                                                                 \
  /* Not implemented! */                                                       \
  j _C_LABEL(name);                                                            \
  END(name)

#endif /* !__ASSEMBLER__ */

#endif /* !_RISCV_SYSCALL_H_ */
