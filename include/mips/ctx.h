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

#endif

#endif
