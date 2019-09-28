#ifndef __MIPS_EXC_H__
#define __MIPS_EXC_H__

#include <mips/mips.h>

#ifdef __ASSEMBLER__

#include <mips/asm.h>

#define SAVE_REG(reg, offset, base) sw reg, (EXC_##offset)(base)

#define LOAD_REG(reg, offset, base) lw reg, (EXC_##offset)(base)

#define SAVE_FPU_REG(reg, offset, base) swc1 reg, (EXC_FPU_##offset)(base)

#define LOAD_FPU_REG(reg, offset, base) lwc1 reg, (EXC_FPU_##offset)(base)

#define SAVE_CPU_CTX(_sp, reg)                                                 \
  SAVE_REG(AT, AT, reg);                                                       \
  SAVE_REG(v0, V0, reg);                                                       \
  SAVE_REG(v1, V1, reg);                                                       \
  SAVE_REG(a0, A0, reg);                                                       \
  SAVE_REG(a1, A1, reg);                                                       \
  SAVE_REG(a2, A2, reg);                                                       \
  SAVE_REG(a3, A3, reg);                                                       \
  SAVE_REG(t0, T0, reg);                                                       \
  SAVE_REG(t1, T1, reg);                                                       \
  SAVE_REG(t2, T2, reg);                                                       \
  SAVE_REG(t3, T3, reg);                                                       \
  SAVE_REG(t4, T4, reg);                                                       \
  SAVE_REG(t5, T5, reg);                                                       \
  SAVE_REG(t6, T6, reg);                                                       \
  SAVE_REG(t7, T7, reg);                                                       \
  SAVE_REG(s0, S0, reg);                                                       \
  SAVE_REG(s1, S1, reg);                                                       \
  SAVE_REG(s2, S2, reg);                                                       \
  SAVE_REG(s3, S3, reg);                                                       \
  SAVE_REG(s4, S4, reg);                                                       \
  SAVE_REG(s5, S5, reg);                                                       \
  SAVE_REG(s6, S6, reg);                                                       \
  SAVE_REG(s7, S7, reg);                                                       \
  SAVE_REG(t8, T8, reg);                                                       \
  SAVE_REG(t9, T9, reg);                                                       \
  SAVE_REG(gp, GP, reg);                                                       \
  SAVE_REG(_sp, SP, reg);                                                      \
  SAVE_REG(fp, FP, reg);                                                       \
  SAVE_REG(ra, RA, reg);                                                       \
  mflo t0;                                                                     \
  mfhi t1;                                                                     \
  SAVE_REG(t0, LO, reg);                                                       \
  SAVE_REG(t1, HI, reg);                                                       \
  mfc0 t0, C0_STATUS;                                                          \
  mfc0 t1, C0_CAUSE;                                                           \
  mfc0 t2, C0_BADVADDR;                                                        \
  mfc0 t3, C0_EPC;                                                             \
  SAVE_REG(t0, SR, reg);                                                       \
  SAVE_REG(t1, CAUSE, reg);                                                    \
  SAVE_REG(t2, BADVADDR, reg);                                                 \
  SAVE_REG(t3, PC, reg)

#define LOAD_CPU_CTX(reg)                                                      \
  LOAD_REG(t0, PC, reg);                                                       \
  mtc0 t0, C0_EPC;                                                             \
  LOAD_REG(t0, HI, reg);                                                       \
  LOAD_REG(t1, LO, reg);                                                       \
  mthi t0;                                                                     \
  mtlo t1;                                                                     \
  LOAD_REG(ra, RA, reg);                                                       \
  LOAD_REG(fp, FP, reg);                                                       \
  LOAD_REG(gp, GP, reg);                                                       \
  LOAD_REG(sp, SP, reg);                                                       \
  LOAD_REG(t9, T9, reg);                                                       \
  LOAD_REG(t8, T8, reg);                                                       \
  LOAD_REG(s7, S7, reg);                                                       \
  LOAD_REG(s6, S6, reg);                                                       \
  LOAD_REG(s5, S5, reg);                                                       \
  LOAD_REG(s4, S4, reg);                                                       \
  LOAD_REG(s3, S3, reg);                                                       \
  LOAD_REG(s2, S2, reg);                                                       \
  LOAD_REG(s1, S1, reg);                                                       \
  LOAD_REG(s0, S0, reg);                                                       \
  LOAD_REG(t7, T7, reg);                                                       \
  LOAD_REG(t6, T6, reg);                                                       \
  LOAD_REG(t5, T5, reg);                                                       \
  LOAD_REG(t4, T4, reg);                                                       \
  LOAD_REG(t3, T3, reg);                                                       \
  LOAD_REG(t2, T2, reg);                                                       \
  LOAD_REG(t1, T1, reg);                                                       \
  LOAD_REG(t0, T0, reg);                                                       \
  LOAD_REG(a3, A3, reg);                                                       \
  LOAD_REG(a2, A2, reg);                                                       \
  LOAD_REG(a1, A1, reg);                                                       \
  LOAD_REG(a0, A0, reg);                                                       \
  LOAD_REG(v1, V1, reg);                                                       \
  LOAD_REG(v0, V0, reg);                                                       \
  LOAD_REG(AT, AT, reg)

#define SAVE_FPU_CTX(reg)                                                      \
  SAVE_FPU_REG($f0, F0, reg);                                                  \
  SAVE_FPU_REG($f1, F1, reg);                                                  \
  SAVE_FPU_REG($f2, F2, reg);                                                  \
  SAVE_FPU_REG($f3, F3, reg);                                                  \
  SAVE_FPU_REG($f4, F4, reg);                                                  \
  SAVE_FPU_REG($f5, F5, reg);                                                  \
  SAVE_FPU_REG($f6, F6, reg);                                                  \
  SAVE_FPU_REG($f7, F7, reg);                                                  \
  SAVE_FPU_REG($f8, F8, reg);                                                  \
  SAVE_FPU_REG($f9, F9, reg);                                                  \
  SAVE_FPU_REG($f10, F10, reg);                                                \
  SAVE_FPU_REG($f11, F11, reg);                                                \
  SAVE_FPU_REG($f12, F12, reg);                                                \
  SAVE_FPU_REG($f13, F13, reg);                                                \
  SAVE_FPU_REG($f14, F14, reg);                                                \
  SAVE_FPU_REG($f15, F15, reg);                                                \
  SAVE_FPU_REG($f16, F16, reg);                                                \
  SAVE_FPU_REG($f17, F17, reg);                                                \
  SAVE_FPU_REG($f18, F18, reg);                                                \
  SAVE_FPU_REG($f19, F19, reg);                                                \
  SAVE_FPU_REG($f20, F20, reg);                                                \
  SAVE_FPU_REG($f21, F21, reg);                                                \
  SAVE_FPU_REG($f22, F22, reg);                                                \
  SAVE_FPU_REG($f23, F23, reg);                                                \
  SAVE_FPU_REG($f24, F24, reg);                                                \
  SAVE_FPU_REG($f25, F25, reg);                                                \
  SAVE_FPU_REG($f26, F26, reg);                                                \
  SAVE_FPU_REG($f27, F27, reg);                                                \
  SAVE_FPU_REG($f28, F28, reg);                                                \
  SAVE_FPU_REG($f29, F29, reg);                                                \
  SAVE_FPU_REG($f30, F30, reg);                                                \
  SAVE_FPU_REG($f31, F31, reg);                                                \
  SAVE_FPU_REG($31, FSR, reg)

#define LOAD_FPU_CTX(reg)                                                      \
  LOAD_FPU_REG($f0, F0, reg);                                                  \
  LOAD_FPU_REG($f1, F1, reg);                                                  \
  LOAD_FPU_REG($f2, F2, reg);                                                  \
  LOAD_FPU_REG($f3, F3, reg);                                                  \
  LOAD_FPU_REG($f4, F4, reg);                                                  \
  LOAD_FPU_REG($f5, F5, reg);                                                  \
  LOAD_FPU_REG($f6, F6, reg);                                                  \
  LOAD_FPU_REG($f7, F7, reg);                                                  \
  LOAD_FPU_REG($f8, F8, reg);                                                  \
  LOAD_FPU_REG($f9, F9, reg);                                                  \
  LOAD_FPU_REG($f10, F10, reg);                                                \
  LOAD_FPU_REG($f11, F11, reg);                                                \
  LOAD_FPU_REG($f12, F12, reg);                                                \
  LOAD_FPU_REG($f13, F13, reg);                                                \
  LOAD_FPU_REG($f14, F14, reg);                                                \
  LOAD_FPU_REG($f15, F15, reg);                                                \
  LOAD_FPU_REG($f16, F16, reg);                                                \
  LOAD_FPU_REG($f17, F17, reg);                                                \
  LOAD_FPU_REG($f18, F18, reg);                                                \
  LOAD_FPU_REG($f19, F19, reg);                                                \
  LOAD_FPU_REG($f20, F20, reg);                                                \
  LOAD_FPU_REG($f21, F21, reg);                                                \
  LOAD_FPU_REG($f22, F22, reg);                                                \
  LOAD_FPU_REG($f23, F23, reg);                                                \
  LOAD_FPU_REG($f24, F24, reg);                                                \
  LOAD_FPU_REG($f25, F25, reg);                                                \
  LOAD_FPU_REG($f26, F26, reg);                                                \
  LOAD_FPU_REG($f27, F27, reg);                                                \
  LOAD_FPU_REG($f28, F28, reg);                                                \
  LOAD_FPU_REG($f29, F29, reg);                                                \
  LOAD_FPU_REG($f30, F30, reg);                                                \
  LOAD_FPU_REG($f31, F31, reg);                                                \
  LOAD_FPU_REG($31, FSR, reg)

#else /* !__ASSEMBLER__ */

#include <stdbool.h>

#define CPU_FRAME                                                              \
  struct {                                                                     \
    register_t at;                                                             \
    register_t v0, v1;                                                         \
    register_t a0, a1, a2, a3;                                                 \
    register_t t0, t1, t2, t3, t4, t5, t6, t7;                                 \
    register_t s0, s1, s2, s3, s4, s5, s6, s7;                                 \
    register_t t8, t9;                                                         \
    register_t gp, sp, fp, ra;                                                 \
    register_t lo, hi;                                                         \
    register_t pc, sr, badvaddr, cause;                                        \
  }

#define FPU_FRAME                                                              \
  struct {                                                                     \
    fpregister_t f0, f1, f2, f3, f4, f5, f6, f7;                               \
    fpregister_t f8, f9, f10, f11, f12, f13, f14, f15;                         \
    fpregister_t f16, f17, f18, f19, f20, f21, f22, f23;                       \
    fpregister_t f24, f25, f26, f27, f28, f29, f30, f31;                       \
    register_t fsr;                                                            \
  }

typedef struct cpu_exc_frame {
  CPU_FRAME;
} cpu_exc_frame_t;
typedef struct fpu_exc_frame {
  FPU_FRAME;
} fpu_exc_frame_t;
typedef struct exc_frame {
  CPU_FRAME;
  FPU_FRAME;
} exc_frame_t;

static inline bool kern_mode_p(exc_frame_t *frame) {
  return (frame->sr & SR_KSU_MASK) == SR_KSU_KERN;
}

static inline bool user_mode_p(exc_frame_t *frame) {
  return (frame->sr & SR_KSU_MASK) == SR_KSU_USER;
}

static inline unsigned exc_code(exc_frame_t *frame) {
  return (frame->cause & CR_X_MASK) >> CR_X_SHIFT;
}

#endif

#endif
