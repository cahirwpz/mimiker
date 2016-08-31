#include <asm.h>
#include <mips/m32c0.h>
#include <mips/ctx.h>

    .set noreorder

#
# uint32_t ctx_save(ctx_t *ctx)
#
LEAF(ctx_save)
        sw      $ra, (REG_RA*4)($a0)
        sw      $fp, (REG_FP*4)($a0)
        sw      $sp, (REG_SP*4)($a0)
        sw      $gp, (REG_GP*4)($a0)
        sw      $s0, (REG_S0*4)($a0)
        sw      $s1, (REG_S1*4)($a0)
        sw      $s2, (REG_S2*4)($a0)
        sw      $s3, (REG_S3*4)($a0)
        sw      $s4, (REG_S4*4)($a0)
        sw      $s5, (REG_S5*4)($a0)
        sw      $s6, (REG_S6*4)($a0)
        sw      $s7, (REG_S7*4)($a0)
        mfc0    $t0, C0_USERLOCAL
        sw      $t0, (REG_TCB*4)($a0)

        # return 0
        jr      $ra
        move    $v0, $zero
END(ctx_save)

#
# void ctx_load(const ctx_t *ctx)
#
LEAF(ctx_load)
        lw      $ra, (REG_RA*4)($a0)
        lw      $fp, (REG_FP*4)($a0)
        lw      $sp, (REG_SP*4)($a0)
        lw      $gp, (REG_GP*4)($a0)
        lw      $s0, (REG_S0*4)($a0)
        lw      $s1, (REG_S1*4)($a0)
        lw      $s2, (REG_S2*4)($a0)
        lw      $s3, (REG_S3*4)($a0)
        lw      $s4, (REG_S4*4)($a0)
        lw      $s5, (REG_S5*4)($a0)
        lw      $s6, (REG_S6*4)($a0)
        lw      $s7, (REG_S7*4)($a0)
        lw      $t0, (REG_TCB*4)($a0)
        mtc0    $t0, C0_USERLOCAL

        # Note that the following will not return from ctx_load, because $ra
        # has been overwritten with the value previously stored by ctx_store.
        # Instead, it resumes the context, and returns 1 there.
        jr      $ra
        li      $v0, 1
END(ctx_load)

# vim: sw=8 ts=8 et
