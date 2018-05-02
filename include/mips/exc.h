#ifndef __MIPS_EXC_H__
#define __MIPS_EXC_H__

#include <mips/mips.h>

#ifdef __ASSEMBLER__

#include <mips/asm.h>

#define SAVE_REG(reg, offset, base) sw reg, (EXC_##offset)(base)

#define LOAD_REG(reg, offset, base) lw reg, (EXC_##offset)(base)

#define SAVE_FPU_REG(reg, offset, base) swc1 reg, (FPU_CTX_##offset)(base)

#define LOAD_FPU_REG(reg, offset, base) lwc1 reg, (FPU_CTX_##offset)(base)

#else // !__ASSEMBLER__

typedef struct exc_frame {
  reg_t at;
  reg_t v0, v1;
  reg_t a0, a1, a2, a3;
  reg_t t0, t1, t2, t3, t4, t5, t6, t7;
  reg_t s0, s1, s2, s3, s4, s5, s6, s7;
  reg_t t8, t9;
  reg_t gp, sp, fp, ra;
  reg_t lo, hi;
  reg_t pc, sr, badvaddr, cause;
} exc_frame_t;

#endif

#endif
