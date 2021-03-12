#include <mips/asm.h>
#include <mips/m32c0.h>
#include <mips/pcpu.h>
#include <mips/regdef.h>

#include "assym.h"

        .set noreorder
        # Enable FPU instructions use since we need to save & restore FP context
        .set    hardfloat

        .local ctx_resume
        .local ctx_save

#define SAVE_REG_CFI(reg, offset, base)                                        \
        sw reg, (CTX_##offset)(base);                                          \
        .cfi_rel_offset reg, (CTX_##offset)

#define SAVE_REG(reg, offset, base) sw reg, (CTX_##offset)(base)

#define LOAD_REG(reg, offset, base) lw reg, (CTX_##offset)(base)

#define SAVE_FPU_REG(reg, offset, base) swc1 reg, (FPU_CTX_##offset)(base)

#define SAVE_CTX(_sr)                                                          \
        SAVE_REG(_sr, SR, sp);                                                 \
        SAVE_REG_CFI(ra, PC, sp);                                              \
        SAVE_REG_CFI(fp, FP, sp);                                              \
        SAVE_REG_CFI(gp, GP, sp);                                              \
        SAVE_REG(zero, V0, sp);                                                \
        SAVE_REG_CFI(s0, S0, sp);                                              \
        SAVE_REG_CFI(s1, S1, sp);                                              \
        SAVE_REG_CFI(s2, S2, sp);                                              \
        SAVE_REG_CFI(s3, S3, sp);                                              \
        SAVE_REG_CFI(s4, S4, sp);                                              \
        SAVE_REG_CFI(s5, S5, sp);                                              \
        SAVE_REG_CFI(s6, S6, sp);                                              \
        SAVE_REG_CFI(s7, S7, sp)

#define SAVE_FPU_CTX(dst)                                                      \
        SAVE_FPU_REG($f0, F0, dst);                                            \
        SAVE_FPU_REG($f1, F1, dst);                                            \
        SAVE_FPU_REG($f2, F2, dst);                                            \
        SAVE_FPU_REG($f3, F3, dst);                                            \
        SAVE_FPU_REG($f4, F4, dst);                                            \
        SAVE_FPU_REG($f5, F5, dst);                                            \
        SAVE_FPU_REG($f6, F6, dst);                                            \
        SAVE_FPU_REG($f7, F7, dst);                                            \
        SAVE_FPU_REG($f8, F8, dst);                                            \
        SAVE_FPU_REG($f9, F9, dst);                                            \
        SAVE_FPU_REG($f10, F10, dst);                                          \
        SAVE_FPU_REG($f11, F11, dst);                                          \
        SAVE_FPU_REG($f12, F12, dst);                                          \
        SAVE_FPU_REG($f13, F13, dst);                                          \
        SAVE_FPU_REG($f14, F14, dst);                                          \
        SAVE_FPU_REG($f15, F15, dst);                                          \
        SAVE_FPU_REG($f16, F16, dst);                                          \
        SAVE_FPU_REG($f17, F17, dst);                                          \
        SAVE_FPU_REG($f18, F18, dst);                                          \
        SAVE_FPU_REG($f19, F19, dst);                                          \
        SAVE_FPU_REG($f20, F20, dst);                                          \
        SAVE_FPU_REG($f21, F21, dst);                                          \
        SAVE_FPU_REG($f22, F22, dst);                                          \
        SAVE_FPU_REG($f23, F23, dst);                                          \
        SAVE_FPU_REG($f24, F24, dst);                                          \
        SAVE_FPU_REG($f25, F25, dst);                                          \
        SAVE_FPU_REG($f26, F26, dst);                                          \
        SAVE_FPU_REG($f27, F27, dst);                                          \
        SAVE_FPU_REG($f28, F28, dst);                                          \
        SAVE_FPU_REG($f29, F29, dst);                                          \
        SAVE_FPU_REG($f30, F30, dst);                                          \
        SAVE_FPU_REG($f31, F31, dst);                                          \
        SAVE_FPU_REG($31, FSR, dst)

#define LOAD_CTX(_sr)                                                          \
        LOAD_REG(_sr, SR, sp);                                                 \
        LOAD_REG(ra, PC, sp);                                                  \
        LOAD_REG(fp, FP, sp);                                                  \
        LOAD_REG(gp, GP, sp);                                                  \
        LOAD_REG(v0, V0, sp);                                                  \
        LOAD_REG(s0, S0, sp);                                                  \
        LOAD_REG(s1, S1, sp);                                                  \
        LOAD_REG(s2, S2, sp);                                                  \
        LOAD_REG(s3, S3, sp);                                                  \
        LOAD_REG(s4, S4, sp);                                                  \
        LOAD_REG(s5, S5, sp);                                                  \
        LOAD_REG(s6, S6, sp);                                                  \
        LOAD_REG(s7, S7, sp)

# Remove given bits from the status register.
#define STATUS_CLEAR(mask)                                                     \
        mfc0    k0, C0_STATUS;                                                 \
        li      k1, ~(mask);                                                   \
        and     k0, k1;                                                        \
        mtc0    k0, C0_STATUS

# Add given bits to the status register.
#define STATUS_SET(mask)                                                       \
        mfc0    k0, C0_STATUS;                                                 \
        li      k1, mask;                                                      \
        or      k0, k1;                                                        \
        mtc0    k0, C0_STATUS

#
# long ctx_switch(thread_t *from, thread_t *to)
#
NESTED(ctx_switch, CTX_SIZE, ra)
        # ctx_switch must be called with interrupts disabled
        mfc0    t0, C0_SR
        andi    t1, t0, SR_IE
        tnei    t1, 0

ctx_save:
        # save context of @from thread
        subu    sp, CTX_SIZE
        .cfi_def_cfa_offset CTX_SIZE
        SAVE_CTX(t0)
        sw      sp, TD_KCTX(a0)

        /* save FPU context only
         * if it needs saving and has not already been saved */
        lw      t2, TD_PFLAGS(a0)
        and     t3, t2, TDP_FPUINUSE|TDP_FPUCTXSAVED
        li      t1, TDP_FPUINUSE
        bne     t1, t3, skip_fpu_save

        # mark FPU context as saved
        or      t2, TDP_FPUCTXSAVED # delay slot
        sw      t2, TD_PFLAGS(a0)

        # ... and finally save it (temporarily enabling FPU)
        STATUS_SET(SR_CU1)
        lw      t2, TD_UCTX(a0)
        SAVE_FPU_CTX(t2)
        STATUS_CLEAR(SR_CU1)

skip_fpu_save:
        move    s1, a1                  # save @from thread pointer

ctx_resume:
        # switch stack pointer to @to thread
        lw      sp, TD_KCTX(s1)

        # update curthread pointer to reference @to thread
        LOAD_PCPU(t0)
        sw      s1, PCPU_CURTHREAD(t0)

        # switch user space if necessary
        jal     vm_map_switch
        move    a0, s1

        # restore @to thread context
        LOAD_CTX(t0)
        addu    sp, CTX_SIZE
        .cfi_def_cfa_offset 0

        # restore status register with updated interrupt mask
        mfc0    t1, C0_SR
        ext     t1, t1, SR_IMASK_SHIFT, SR_IMASK_BITS
        ins     t0, t1, SR_IMASK_SHIFT, SR_IMASK_BITS
        mtc0    t0, C0_SR

        jr.hb   ra
        nop
END(ctx_switch)

# vim: sw=8 ts=8 et
