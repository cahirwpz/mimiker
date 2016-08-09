#include "asm.h"

    .set noreorder

/* uint32_t ctx_save(ctx_t *ctx) */
LEAF(ctx_save)

    sw $ra, ( 0*4)($a0)
    sw $fp, ( 1*4)($a0)
    sw $sp, ( 2*4)($a0)
    sw $gp, ( 3*4)($a0)
    sw $s0, ( 4*4)($a0)
    sw $s1, ( 5*4)($a0)
    sw $s2, ( 6*4)($a0)
    sw $s3, ( 7*4)($a0)
    sw $s4, ( 8*4)($a0)
    sw $s5, ( 9*4)($a0)
    sw $s6, (10*4)($a0)
    sw $s7, (11*4)($a0)

    /* Return 0 */
    jr $ra
    move $v0, $zero

END(ctx_save)

/* void ctx_load(const ctx_t *ctx) */
LEAF(ctx_load)

    lw $ra, ( 0*4)($a0)
    lw $fp, ( 1*4)($a0)
    lw $sp, ( 2*4)($a0)
    lw $gp, ( 3*4)($a0)
    lw $s0, ( 4*4)($a0)
    lw $s1, ( 5*4)($a0)
    lw $s2, ( 6*4)($a0)
    lw $s3, ( 7*4)($a0)
    lw $s4, ( 8*4)($a0)
    lw $s5, ( 9*4)($a0)
    lw $s6, (10*4)($a0)
    lw $s7, (11*4)($a0)

    /*
     * Note that the following will not return from ctx_load, because
     * $ra has been overwritten with the value previously stored by
     * ctx_store. Instead, it resumes the context, and returns 1
     * there.
     */
    jr $ra
    li $v0, 1 # is this needed?

END(ctx_load)

/* void ctx_call(const ctx_t *ctx, void *fn) */
LEAF(ctx_call)

    lw $ra, ( 0*4)($a0)
    lw $fp, ( 1*4)($a0)
    lw $sp, ( 2*4)($a0)
    lw $gp, ( 3*4)($a0)
    lw $s0, ( 4*4)($a0)
    lw $s1, ( 5*4)($a0)
    lw $s2, ( 6*4)($a0)
    lw $s3, ( 7*4)($a0)
    lw $s4, ( 8*4)($a0)
    lw $s5, ( 9*4)($a0)
    lw $s6, (10*4)($a0)
    lw $s7, (11*4)($a0)

    jr $a1
    li $v0, 1

END(ctx_call)

/* void ctx_load_interrupt(const ctx_t *ctx) */
LEAF(ctx_load_interrupt)

    lw $ra, ( 0*4)($a0)
    lw $fp, ( 1*4)($a0)
    lw $sp, ( 2*4)($a0)
    lw $gp, ( 3*4)($a0)
    lw $s0, ( 4*4)($a0)
    lw $s1, ( 5*4)($a0)
    lw $s2, ( 6*4)($a0)
    lw $s3, ( 7*4)($a0)
    lw $s4, ( 8*4)($a0)
    lw $s5, ( 9*4)($a0)
    lw $s6, (10*4)($a0)
    lw $s7, (11*4)($a0)

    /*
     * Note that the following will not return from ctx_load, because
     * $ra has been overwritten with the value previously stored by
     * ctx_store. Instead, it resumes the context.
     */

    mtc0 $ra, $14
    ei

    eret
    nop

END(ctx_load_interrupt)
