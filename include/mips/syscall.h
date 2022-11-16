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
  bnez v1, 1f;                                                                 \
  j ra;                                                                        \
  1 : j _C_LABEL(__sc_error);                                                  \
  END(name)

#endif /* !__ASSEMBLER__ */

#if defined(_MACHDEP) && defined(_KERNEL)

static inline register_t sc_md_code(ctx_t *ctx) {
  return _REG(ctx, V0);
}

static inline void *sc_md_args(ctx_t *ctx) {
  return &_REG(ctx, A0);
}

/*
 * From ABI:
 * Despite the fact that some or all of the arguments to a function are
 * passed in registers, always allocate space on the stack for all
 * arguments.
 * For this reason, we read from the user stack with some offset.
 */
static inline void *sc_md_stack_args(ctx_t *ctx, size_t nregs) {
  return (void *)(_REG(ctx, SP) + nregs * sizeof(register_t));
}

#endif /* !_MACHDEP && !_KERNEL */

#endif /* !_MIPS_SYSCALL_H_ */
