#include <asm.h>
#include <mips/m32c0.h>
#include <mips/ctx.h>

#include "genassym.h"

    .set noreorder
    .set noat

#
# void ctx_switch(ctx_t *from, ctx_t *to)
#
LEAF(ctx_switch)
        sw      $ra, (REG_PC*4)($a0)
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

        lw      $t0, (REG_TCB*4)($a1)
        mtc0    $t0, C0_USERLOCAL
        lw      $ra, (REG_PC*4)($a1)
        lw      $fp, (REG_FP*4)($a1)
        lw      $sp, (REG_SP*4)($a1)
        lw      $gp, (REG_GP*4)($a1)
        lw      $s0, (REG_S0*4)($a1)
        lw      $s1, (REG_S1*4)($a1)
        lw      $s2, (REG_S2*4)($a1)
        lw      $s3, (REG_S3*4)($a1)
        lw      $s4, (REG_S4*4)($a1)
        lw      $s5, (REG_S5*4)($a1)
        lw      $s6, (REG_S6*4)($a1)
        lw      $s7, (REG_S7*4)($a1)

        jr      $ra
        nop
END(ctx_switch)

#
# noreturn void ctx_boot(const ctx_t *ctx)
#
LEAF(ctx_boot)
        move    $t0, $a0

        lw      $ra, (REG_RA*4)($t0)
        lw      $fp, (REG_FP*4)($t0)
        lw      $sp, (REG_SP*4)($t0)
        lw      $gp, (REG_GP*4)($t0)
        lw      $a0, (REG_A0*4)($t0)
        lw      $a1, (REG_A1*4)($t0)
        lw      $a2, (REG_A2*4)($t0)
        lw      $a3, (REG_A3*4)($t0)

        lw      $t1, (REG_TCB*4)($t0)
        mtc0    $t1, C0_USERLOCAL

        lw      $t1, (REG_PC*4)($t0)
        jr      $t1
        nop
END(ctx_boot)

# vim: sw=8 ts=8 et
