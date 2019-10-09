#ifndef _MIPS_SYSCALL_H_
#define _MIPS_SYSCALL_H_

#ifdef __ASSEMBLER__

#include <mips/asm.h>
#include <mips/regdef.h>
#include <sys/errno.h>
#include <sys/syscall.h>

/* Do a syscall that is missing an implementation. */
#define SYSCALL_MISSING(name)                                                  \
  LEAF(name);                                                                  \
  j _C_LABEL(__sc_missing);                                                    \
  END(name)

/* Do a syscall that cannot fail. */
#define SYSCALL_NOERROR(name, num)                                             \
  LEAF(name);                                                                  \
  li v0, num;                                                                  \
  syscall;                                                                     \
  j ra;                                                                        \
  END(name)

/* Do a normal syscall. */
#define SYSCALL(name, num)                                                     \
  LEAF(name);                                                                  \
  li v0, num;                                                                  \
  syscall;                                                                     \
  bnez v1, _C_LABEL(__sc_error);                                               \
  j ra;                                                                        \
  END(name)

#endif /* !__ASSEMBLER__ */

#endif /* !_MIPS_SYSCALL_H_ */
