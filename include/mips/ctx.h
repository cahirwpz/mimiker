#ifndef __MIPS_CTX_H__
#define __MIPS_CTX_H__

#include <mips/mips.h>

#ifdef __ASSEMBLER__

#include <mips/asm.h>

#define SAVE_REG(reg, offset, base) sw reg, (CTX_##offset)(base)

#define LOAD_REG(reg, offset, base) lw reg, (CTX_##offset)(base)

#define SAVE_CTX(_sr, _sp, reg)                                                \
  SAVE_REG(_sr, SR, reg);                                                      \
  SAVE_REG(ra, PC, reg);                                                       \
  SAVE_REG(fp, FP, reg);                                                       \
  SAVE_REG(_sp, SP, reg);                                                      \
  SAVE_REG(gp, GP, reg);                                                       \
  SAVE_REG(zero, V0, reg);                                                     \
  SAVE_REG(s0, S0, reg);                                                       \
  SAVE_REG(s1, S1, reg);                                                       \
  SAVE_REG(s2, S2, reg);                                                       \
  SAVE_REG(s3, S3, reg);                                                       \
  SAVE_REG(s4, S4, reg);                                                       \
  SAVE_REG(s5, S5, reg);                                                       \
  SAVE_REG(s6, S6, reg);                                                       \
  SAVE_REG(s7, S7, reg)

#define LOAD_CTX(_sr, reg)                                                     \
  LOAD_REG(_sr, SR, reg);                                                      \
  LOAD_REG(ra, PC, reg);                                                       \
  LOAD_REG(fp, FP, reg);                                                       \
  LOAD_REG(sp, SP, reg);                                                       \
  LOAD_REG(gp, GP, reg);                                                       \
  LOAD_REG(v0, V0, reg);                                                       \
  LOAD_REG(s0, S0, reg);                                                       \
  LOAD_REG(s1, S1, reg);                                                       \
  LOAD_REG(s2, S2, reg);                                                       \
  LOAD_REG(s3, S3, reg);                                                       \
  LOAD_REG(s4, S4, reg);                                                       \
  LOAD_REG(s5, S5, reg);                                                       \
  LOAD_REG(s6, S6, reg);                                                       \
  LOAD_REG(s7, S7, reg)

#else /* !__ASSEMBLER__ */

typedef struct ctx {
  register_t s0, s1, s2, s3, s4, s5, s6, s7;
  register_t v0, gp, sp, fp, ra;
  register_t pc, sr;
} ctx_t;

#endif

#endif
