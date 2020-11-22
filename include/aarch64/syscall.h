#ifndef _AARCH64_SYSCALL_H_
#define _AARCH64_SYSCALL_H_

#ifdef __ASSEMBLER__

#include <aarch64/asm.h>
#include <sys/errno.h>
#include <sys/syscall.h>
/* Do a syscall that is missing an implementation. */
#define SYSCALL_MISSING(name)                                                  \
  ENTRY(name);                                                                 \
  b _C_LABEL(__sc_missing);                                                    \
  END(name)

/* Do a syscall that cannot fail. */
#define SYSCALL_NOERROR(name, num)                                             \
  ENTRY(name);                                                                 \
  svc num;                                                                     \
  ret;                                                                         \
  END(name)

/* Do a normal syscall. */
/* TODO(pj): Handle errors. */
#define SYSCALL(name, num)                                                     \
  ENTRY(name);                                                                 \
  svc num;                                                                     \
  ret;                                                                         \
  END(name)

#endif /* !__ASSEMBLER__ */

#endif /* !_AARCH64_SYSCALL_H_ */
