#include <mips/asm.h>
#include <mips/m32c0.h>
#include <mips/pcpu.h>
#include <mips/regdef.h>

#include "assym.h"

        .set noreorder

        .local ctx_resume
        .local ctx_save

#define SAVE_REG(reg, offset, base) sw reg, (CTX_##offset)(base)

#define LOAD_REG(reg, offset, base) lw reg, (CTX_##offset)(base)

#define SAVE_CTX(_sr, _sp, reg)                                                \
        SAVE_REG(_sr, SR, reg);                                                \
        SAVE_REG(ra, PC, reg);                                                 \
        SAVE_REG(fp, FP, reg);                                                 \
        SAVE_REG(_sp, SP, reg);                                                \
        SAVE_REG(gp, GP, reg);                                                 \
        SAVE_REG(zero, V0, reg);                                               \
        SAVE_REG(s0, S0, reg);                                                 \
        SAVE_REG(s1, S1, reg);                                                 \
        SAVE_REG(s2, S2, reg);                                                 \
        SAVE_REG(s3, S3, reg);                                                 \
        SAVE_REG(s4, S4, reg);                                                 \
        SAVE_REG(s5, S5, reg);                                                 \
        SAVE_REG(s6, S6, reg);                                                 \
        SAVE_REG(s7, S7, reg)

#define LOAD_CTX(_sr, reg)                                                     \
        LOAD_REG(_sr, SR, reg);                                                \
        LOAD_REG(ra, PC, reg);                                                 \
        LOAD_REG(fp, FP, reg);                                                 \
        LOAD_REG(sp, SP, reg);                                                 \
        LOAD_REG(gp, GP, reg);                                                 \
        LOAD_REG(v0, V0, reg);                                                 \
        LOAD_REG(s0, S0, reg);                                                 \
        LOAD_REG(s1, S1, reg);                                                 \
        LOAD_REG(s2, S2, reg);                                                 \
        LOAD_REG(s3, S3, reg);                                                 \
        LOAD_REG(s4, S4, reg);                                                 \
        LOAD_REG(s5, S5, reg);                                                 \
        LOAD_REG(s6, S6, reg);                                                 \
        LOAD_REG(s7, S7, reg)

#
# long ctx_switch(thread_t *from, thread_t *to)
#
NESTED(ctx_switch, CTX_SIZE, ra)
        # ctx_switch must be called with interrupts disabled
        mfc0    t0, C0_SR
        andi    t1, t0, SR_IE
        tnei    t1, 0

        # don't save context of @from thread if user did not provide one
        bnez    a0, ctx_save
        nop
        j       ctx_resume
        move    s1, a1                  # (delay) save @to thread pointer

ctx_save:
        # save context of @from thread
        move    t1, sp
        subu    sp, CTX_SIZE
        SAVE_CTX(t0, t1, sp)
        sw      sp, TD_KCTX(a0)

        move    s0, a0                  # save @from pointer

        # disable interrupts so interlock on td_spin can be done safely
        jal     intr_disable
        move    s1, a1                  # (delay) save @to thread pointer

        # release @from thread spin lock
        jal     spin_unlock
        addu    a0, s0, TD_LOCK         # (delay) 1st arg - @from spin lock

ctx_resume:
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
        addu    a0, s1, TD_LOCK         # (delay) 1st arg - @to spin lock

        # Enable interrupts finishing safe interlock on td_spin.
        jal     intr_enable
        nop

        # switch user space if necessary
1:      lw      a0, TD_PROC(s1)
        beqz    a0, 2f                  # switching to kernel thread ?
        nop
        lw      a0, P_USPACE(a0)
2:      jal     vm_map_activate
        nop

        # restore @to thread context
        lw      t1, TD_KCTX(s1)
        LOAD_CTX(t0, t1)

        # restore status register with updated interrupt mask
        mfc0    t1, C0_SR
        ext     t1, t1, SR_IMASK_SHIFT, SR_IMASK_BITS
        ins     t0, t1, SR_IMASK_SHIFT, SR_IMASK_BITS
        mtc0    t0, C0_SR

        jr.hb   ra
        nop
END(ctx_switch)

# vim: sw=8 ts=8 et
