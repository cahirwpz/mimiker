#include <mips/asm.h>
#include <mips/m32c0.h>
#include <mips/ctx.h>
#include <mips/pcpu.h>
#include <mips/regdef.h>

#include "assym.h"

        .set noreorder

        .global ctx_switch
        .local  ctx_resume
        .local  ctx_save

#
# void ctx_switch(thread_t *from, thread_t *to)
#
LEAF(ctx_switch)
        # enter exception level (prevents interrupts from being enabled)
        mfc0    t0, C0_SR               # $t0 = C0_SR
        ori     t1, t0, SR_EXL
        mtc0    t1, C0_SR

        # don't save context of @from thread if user did not provide one
        bnez    a0, ctx_save
        addu    t1, a0, TD_KCTX         # (delay) load context pointer
        j       ctx_resume
        move    s0, a1                  # (delay) save @to thread pointer

ctx_save:
        # save @from context
        SAVE_REG(t0, SR, t1)
        SAVE_REG(ra, PC, t1)
        SAVE_REG(fp, FP, t1)
        SAVE_REG(sp, SP, t1)
        SAVE_REG(gp, GP, t1)
        SAVE_REG(s0, S0, t1)
        SAVE_REG(s1, S1, t1)
        SAVE_REG(s2, S2, t1)
        SAVE_REG(s3, S3, t1)
        SAVE_REG(s4, S4, t1)
        SAVE_REG(s5, S5, t1)
        SAVE_REG(s6, S6, t1)
        SAVE_REG(s7, S7, t1)

        # release @from thread spin lock
        addu    a0, a0, TD_SPIN
        jal     spin_release
        move    s0, a1                  # (delay) save @to thread pointer

ctx_resume:
        # updated curthread pointer to reference @to thread
        LOAD_PCPU(t0)
        sw      s0, PCPU_CURTHREAD(t0)

        # check for TDF_NEEDLOCK flag
        lw      t0, TD_FLAGS(s0)
        andi    t0, TDF_NEEDLOCK
        beqz    t0, 1f
        nop

        # acquire @to thread spin lock
        addu    a0, s0, TD_SPIN
        la      a1, ctx_save
        jal     _spin_acquire
        nop

        # switch user space if necessary
1:      lw      a0, TD_PROC(s0)
        beqz    a0, 1f                  # switching to kernel thread ?
        nop
        lw      a0, P_USPACE(a0)
1:      jal     vm_map_activate
        nop

        # restore @to thread context
        addu    a1, s0, TD_KCTX
        LOAD_REG(ra, PC, a1)
        LOAD_REG(fp, FP, a1)
        LOAD_REG(sp, SP, a1)
        LOAD_REG(gp, GP, a1)
        LOAD_REG(s0, S0, a1)
        LOAD_REG(s1, S1, a1)
        LOAD_REG(s2, S2, a1)
        LOAD_REG(s3, S3, a1)
        LOAD_REG(s4, S4, a1)
        LOAD_REG(s5, S5, a1)
        LOAD_REG(s6, S6, a1)
        LOAD_REG(s7, S7, a1)

        LOAD_REG(t0, SR, a1)            # restore C0_SR except interrupt mask
        and     t0, t0, ~SR_IMASK
        mfc0    t1, C0_SR
        and     t1, t1, SR_IMASK
        or      t0, t0, t1
        mtc0    t0, C0_SR

        jr.hb   ra
        nop
END(ctx_switch)

# vim: sw=8 ts=8 et
