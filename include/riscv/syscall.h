#ifndef _RISCV_SYSCALL_H_
#define _RISCV_SYSCALL_H_

#ifdef __ASSEMBLER__

#include <sys/syscall.h>
#include <riscv/asm.h>

/* Do a syscall that is missing an implementation. */
#define SYSCALL_MISSING(name)                                                  \
  ENTRY(name);                                                                 \
  j _C_LABEL(__sc_missing);                                                    \
  END(name)

/* Do a syscall that cannot fail. */
#define SYSCALL_NOERROR(name, num)                                             \
  ENTRY(name);                                                                 \
  REG_LI a7, num;                                                              \
  ecall;                                                                       \
  ret;                                                                         \
  END(name)

/* Do a normal syscall. */
#define SYSCALL(name, num)                                                     \
  ENTRY(name);                                                                 \
  REG_LI a7, num;                                                              \
  ecall;                                                                       \
  bnez a1, 1f;                                                                 \
  ret;                                                                         \
  1 : j _C_LABEL(__sc_error);                                                  \
  END(name)

#endif /* !__ASSEMBLER__ */

#if defined(_MACHDEP) && defined(_KERNEL)

/*
 * RISC-V syscall ABI:
 *  - a7: code
 *  - a0-5: args
 */
static inline void *sc_md_args(ctx_t *ctx) {
  return &_REG(ctx, A0);
}

#endif /* !_MACHDEP && !_KERNEL */

#endif /* !_RISCV_SYSCALL_H_ */
