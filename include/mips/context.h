#ifndef _MIPS_EXCEPTION_H_
#define _MIPS_EXCEPTION_H_

#ifndef _MACHDEP
#error "Do not use this header file outside kernel machine dependent code!"
#endif

#include <mips/m32c0.h>
#include <mips/mcontext.h>
#include <sys/types.h>
#include <stdbool.h>

typedef struct ctx {
  __gregset_t __gregs;
} ctx_t;

typedef struct user_ctx {
  __gregset_t __gregs;
  __fpregset_t __fpregs;
} user_ctx_t;

static inline bool kern_mode_p(ctx_t *ctx) {
  return (ctx->__gregs[_REG_SR] & SR_KSU_MASK) == SR_KSU_KERN;
}

static inline bool user_mode_p(ctx_t *ctx) {
  return (ctx->__gregs[_REG_SR] & SR_KSU_MASK) == SR_KSU_USER;
}

static inline unsigned exc_code(ctx_t *ctx) {
  return (ctx->__gregs[_REG_CAUSE] & CR_X_MASK) >> CR_X_SHIFT;
}

__noreturn void kernel_oops(ctx_t *ctx);
void tlb_exception_handler(ctx_t *ctx);

#endif /* !_MIPS_EXCEPTION_ */
