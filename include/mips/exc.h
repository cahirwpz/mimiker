#ifndef __MIPS_EXC_H__
#define __MIPS_EXC_H__

#include <mips/mips.h>

#ifdef __ASSEMBLER__

#include <mips/asm.h>

#define SAVE_REG(reg, offset, base) sw reg, (EXC_##offset)(base)

#define LOAD_REG(reg, offset, base) lw reg, (EXC_##offset)(base)

#define SAVE_FPU_REG(reg, offset, base) swc1 reg, (EXC_FPU_##offset)(base)

#define LOAD_FPU_REG(reg, offset, base) lwc1 reg, (EXC_FPU_##offset)(base)

#else // !__ASSEMBLER__

#define CPU_FRAME                                                              \
  struct {                                                                     \
    reg_t at;                                                                  \
    reg_t v0, v1;                                                              \
    reg_t a0, a1, a2, a3;                                                      \
    reg_t t0, t1, t2, t3, t4, t5, t6, t7;                                      \
    reg_t s0, s1, s2, s3, s4, s5, s6, s7;                                      \
    reg_t t8, t9;                                                              \
    reg_t gp, sp, fp, ra;                                                      \
    reg_t lo, hi;                                                              \
    reg_t pc, sr, badvaddr, cause;                                             \
  }

#define FPU_FRAME                                                              \
  struct {                                                                     \
    freg_t f0, f1, f2, f3, f4, f5, f6, f7;                                     \
    freg_t f8, f9, f10, f11, f12, f13, f14, f15;                               \
    freg_t f16, f17, f18, f19, f20, f21, f22, f23;                             \
    freg_t f24, f25, f26, f27, f28, f29, f30, f31;                             \
    reg_t fsr;                                                                 \
  }

typedef struct cpu_exc_frame { CPU_FRAME; } cpu_exc_frame_t;
typedef struct fpu_exc_frame { FPU_FRAME; } fpu_exc_frame_t;
typedef struct exc_frame {
  CPU_FRAME;
  FPU_FRAME;
} exc_frame_t;

#endif

#endif
