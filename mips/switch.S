#include <mips/asm.h>
#include <mips/m32c0.h>
#include <mips/ctx.h>
#include <mips/pcpu.h>

#include "assym.h"

        .set noreorder

        .global ctx_switch
        .local  ctx_resume

#
# void ctx_switch(thread_t *from, thread_t *to)
#
LEAF(ctx_switch)
        di      $t0             # $t0 = C0_SR, C0_SR &= ~SR_IE

        # don't save context of @from thread if user did not provide one
        beqz    $a0, ctx_resume

        addu    $a0, TD_KCTX
        SAVE_REG($t0, SR, $a0)
        SAVE_REG($ra, PC, $a0)
        SAVE_REG($fp, FP, $a0)
        SAVE_REG($sp, SP, $a0)
        SAVE_REG($gp, GP, $a0)
        SAVE_REG($s0, S0, $a0)
        SAVE_REG($s1, S1, $a0)
        SAVE_REG($s2, S2, $a0)
        SAVE_REG($s3, S3, $a0)
        SAVE_REG($s4, S4, $a0)
        SAVE_REG($s5, S5, $a0)
        SAVE_REG($s6, S6, $a0)
        SAVE_REG($s7, S7, $a0)

ctx_resume:
        move    $s0, $a1
        LOAD_PCPU($t0)
        sw      $s0, PCPU_CURTHREAD($t0)

        lw      $a0, TD_PROC($s0)
        beqz    $a0, 1f                # switching to kernel thread ?
        nop
        lw      $a0, P_USPACE($a0)
1:      jal     vm_map_activate
        nop

        addu    $a1, $s0, TD_KCTX
        LOAD_REG($ra, PC, $a1)
        LOAD_REG($fp, FP, $a1)
        LOAD_REG($sp, SP, $a1)
        LOAD_REG($gp, GP, $a1)
        LOAD_REG($s0, S0, $a1)
        LOAD_REG($s1, S1, $a1)
        LOAD_REG($s2, S2, $a1)
        LOAD_REG($s3, S3, $a1)
        LOAD_REG($s4, S4, $a1)
        LOAD_REG($s5, S5, $a1)
        LOAD_REG($s6, S6, $a1)
        LOAD_REG($s7, S7, $a1)

        LOAD_REG($t0, SR, $a1)          # restore C0_SR except interrupt mask
        and     $t0, $t0, ~SR_IMASK
        mfc0    $t1, C0_SR
        and     $t1, $t1, SR_IMASK
        or      $t0, $t0, $t1
        mtc0    $t0, C0_SR

        jr.hb   $ra
        nop
END(ctx_switch)

# vim: sw=8 ts=8 et
