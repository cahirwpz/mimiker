#include "asm.h"

    .set noreorder  // Disable automatic instruction reordering


/* uint32_t ctx_store(struct ctx_struct* ctx) */
LEAF(ctx_store)

    sw $ra, ( 0*4)($a0)
    sw $sp, ( 1*4)($a0)
    sw $gp, ( 2*4)($a0)
    sw $s0, ( 3*4)($a0)
    sw $s1, ( 4*4)($a0)
    sw $s2, ( 5*4)($a0)
    sw $s3, ( 6*4)($a0)
    sw $s4, ( 7*4)($a0)
    sw $s5, ( 8*4)($a0)
    sw $s6, ( 9*4)($a0)
    sw $s7, (10*4)($a0)
    sw $s8, (11*4)($a0)

    /* Return 0 */
    jr $ra
    move $v0, $zero
END(ctx_store)


/* void ctx_load(const struct ctx_struct* ctx) */
LEAF(ctx_load)

    lw $ra, ( 0*4)($a0)
    lw $sp, ( 1*4)($a0)
    lw $gp, ( 2*4)($a0)
    lw $s0, ( 3*4)($a0)
    lw $s1, ( 4*4)($a0)
    lw $s2, ( 5*4)($a0)
    lw $s3, ( 6*4)($a0)
    lw $s4, ( 7*4)($a0)
    lw $s5, ( 8*4)($a0)
    lw $s6, ( 9*4)($a0)
    lw $s7, (10*4)($a0)
    lw $s8, (11*4)($a0)

    /* Note that the following will not return from ctx_load, because
	 * $ra has been overwritten with the value previously stored by
     * ctx_store. Instead, it resumes the context, and returns 1
     * there.*/
    jr $ra
    li $v0, 1
END(ctx_load)
