#include <mips/asm.h>
#include <mips/m32c0.h>
#include <mips/pcpu.h>
#include <mips/regdef.h>

#include "assym.h"

        .set noreorder

        .local ctx_resume
        .local ctx_save

#define SAVE_REG_CFI(reg, offset, base)                                        \
        sw reg, (CTX_##offset)(base);                                          \
        .cfi_rel_offset reg, (CTX_##offset)

#define SAVE_REG(reg, offset, base) sw reg, (CTX_##offset)(base)

#define LOAD_REG(reg, offset, base) lw reg, (CTX_##offset)(base)

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

        move    s1, a1                  # save @from thread pointer

ctx_resume:
        # switch stack pointer to @to thread
        lw      sp, TD_KCTX(s1)

        # update curthread pointer to reference @to thread
        LOAD_PCPU(t0)
        sw      s1, PCPU_CURTHREAD(t0)

        # check for TDF_NEEDLOCK flag
        lw      t0, TD_FLAGS(s1)
        andi    t0, TDF_NEEDLOCK
        beqz    t0, 1f
        nop

        # acquire @to thread spin lock 
        la      a1, ctx_resume          # 2nd arg - waiting point
        jal     _spin_lock
        lw      a0, TD_LOCK(s1)         # (delay) 1st arg - @to spin lock

        # Enable interrupts finishing safe interlock on td_spin.
        jal     intr_enable
        nop

        # switch user space if necessary
1:      jal     vm_map_switch
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
