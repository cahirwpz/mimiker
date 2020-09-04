#ifndef _MIPS_EXCEPTION_H_
#define _MIPS_EXCEPTION_H_

#ifndef _MACHDEP
#error "Do not use this header file outside kernel machine dependent code!"
#endif

#include <mips/m32c0.h>
#include <sys/types.h>
#include <stdbool.h>

#define CPU_CTX                                                                \
  struct {                                                                     \
    register_t at;                                                             \
    register_t v0, v1;                                                         \
    register_t a0, a1, a2, a3;                                                 \
    register_t t0, t1, t2, t3, t4, t5, t6, t7;                                 \
    register_t s0, s1, s2, s3, s4, s5, s6, s7;                                 \
    register_t t8, t9;                                                         \
    register_t k0, k1;                                                         \
    register_t gp, sp, fp, ra;                                                 \
    register_t lo, hi;                                                         \
    register_t cause, pc, sr, badvaddr;                                        \
  }

#define FPU_CTX                                                                \
  struct {                                                                     \
    fpregister_t f0, f1, f2, f3, f4, f5, f6, f7;                               \
    fpregister_t f8, f9, f10, f11, f12, f13, f14, f15;                         \
    fpregister_t f16, f17, f18, f19, f20, f21, f22, f23;                       \
    fpregister_t f24, f25, f26, f27, f28, f29, f30, f31;                       \
    register_t fsr;                                                            \
  }

typedef struct ctx {
  CPU_CTX;
} ctx_t;

typedef struct user_ctx {
  CPU_CTX;
  FPU_CTX;
} user_ctx_t;

#undef FPU_CTX
#undef CPU_CTX

static inline bool kern_mode_p(ctx_t *ctx) {
  return (ctx->sr & SR_KSU_MASK) == SR_KSU_KERN;
}

static inline bool user_mode_p(ctx_t *ctx) {
  return (ctx->sr & SR_KSU_MASK) == SR_KSU_USER;
}

static inline unsigned exc_code(ctx_t *ctx) {
  return (ctx->cause & CR_X_MASK) >> CR_X_SHIFT;
}

__noreturn void kernel_oops(ctx_t *ctx);

#endif /* !_MIPS_EXCEPTION_ */
