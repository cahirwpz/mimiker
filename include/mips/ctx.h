#ifndef __MIPS_CTX_H__
#define __MIPS_CTX_H__

#include <mips/mips.h>

#ifdef __ASSEMBLER__

#include <mips/asm.h>

#define SAVE_REG(reg, offset, base) sw reg, (CTX_##offset)(base)

#define LOAD_REG(reg, offset, base) lw reg, (CTX_##offset)(base)

#else /* !__ASSEMBLER__ */

typedef struct ctx {
  reg_t s0, s1, s2, s3, s4, s5, s6, s7;
  reg_t gp, sp, fp, ra;
  reg_t pc, sr;
} ctx_t;

/* Following registers are saved only for user threads. */
typedef struct fpu_ctx {
  freg_t f0, f1, f2, f3, f4, f5, f6, f7;
  freg_t f8, f9, f10, f11, f12, f13, f14, f15;
  freg_t f16, f17, f18, f19, f20, f21, f22, f23;
  freg_t f24, f25, f26, f27, f28, f29, f30, f31;
  reg_t fsr;
} fpu_ctx_t;

#endif

#endif
