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
#define SYSCALL(name, num)                                                     \
  ENTRY(name);                                                                 \
  svc num;                                                                     \
  cbnz x1, _C_LABEL(__sc_error);                                               \
  ret;                                                                         \
  END(name)

#endif /* !__ASSEMBLER__ */

#if defined(_MACHDEP) && defined(_KERNEL)

static inline void *sc_md_args(ctx_t *ctx) {
  return &_REG(ctx, X0);
}

static inline void *sc_md_stack_args(ctx_t *ctx, size_t nregs) {
  panic("not implemented!");
}

#endif /* !_MACHDEP && !_KERNEL */

#endif /* !_AARCH64_SYSCALL_H_ */
