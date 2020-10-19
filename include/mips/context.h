#ifndef _MIPS_CONTEXT_H_
#define _MIPS_CONTEXT_H_

#ifndef _MACHDEP
#error "Do not use this header file outside kernel machine dependent code!"
#endif

#include <mips/m32c0.h>
#include <mips/mcontext.h>
#include <sys/context.h>
#include <sys/types.h>
#include <stdbool.h>

typedef struct ctx {
  __gregset_t __gregs;
} ctx_t;

#define _REG(ctx, n) ((ctx)->__gregs[_REG_##n])

static inline bool kern_mode_p(ctx_t *ctx) {
  return (_REG(ctx, SR) & SR_KSU_MASK) == SR_KSU_KERN;
}

static inline bool user_mode_p(ctx_t *ctx) {
  return (_REG(ctx, SR) & SR_KSU_MASK) == SR_KSU_USER;
}

#endif /* !_MIPS_CONTEXT_ */
