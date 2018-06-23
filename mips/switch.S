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
        # disable interrupts saving SR to t0
        di      t0
        ehb

        # don't save context of @from thread if user did not provide one
        bnez    a0, ctx_save
        addu    t1, a0, TD_KCTX         # (delay) load context pointer
        j       ctx_resume
        move    s1, a1                  # (delay) save @to thread pointer

ctx_save:
        # save context of @from thread
        SAVE_REG(t0, SR, t1)
        SAVE_CTX(t1)

        move    s0, a0                  # save @from pointer

        # disable interrupts so interlock on td_spin can be done safely
        jal     intr_disable
        move    s1, a1                  # (delay) save @to thread pointer

        # release @from thread spin lock
        jal     spin_release
        addu    a0, s0, TD_SPIN         # (delay) 1st arg - @from spin lock

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
        jal     _spin_acquire
        addu    a0, s1, TD_SPIN         # (delay) 1st arg - @to spin lock

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
        addu    t1, s1, TD_KCTX
        LOAD_CTX(t1)

        # restore status register with updated interrupt mask
        LOAD_REG(t0, SR, t1)
        mfc0    t1, C0_SR
        ext     t1, SR_IMASK_SHIFT, SR_IMASK_BITS
        ins     t0, t1, SR_IMASK_SHIFT, SR_IMASK_BITS
        mtc0    t0, C0_SR

        jr.hb   ra
        nop
END(ctx_switch)

# vim: sw=8 ts=8 et
