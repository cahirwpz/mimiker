#ifndef _RISCV_PCPU_H_
#define _RISCV_PCPU_H_

#include <sys/context.h>

#define PCPU_MD_FIELDS                                                         \
  struct {                                                                     \
    /* Context saved in copyin/copyout routines. */                            \
    ctx_t *cpy_ctx;                                                            \
  }

#endif /* !_RISCV_PCPU_H_ */
