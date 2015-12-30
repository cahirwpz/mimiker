    .set noreorder // Disable automatic instruction reordering

    /* Interrupts vector */
    .section .exception
    .globl _interrupt_handler
    .org 0x200
_interrupt_handler:
#define REG_STACK_STORE_SIZE 76
    /* Please, no nested interrupts for now. */
    di

    /* Allocate space for register storage on the stack. */
    subu $sp, $sp, REG_STACK_STORE_SIZE

    /* Save registers on the stack. Saving all t,a and v
       registers, as well as LO and HI.
       k registers do not require saving.
       s and sp registers are saved by compiler-generated
       procedure entry, if the procedure modifies them.*/
    sw $t0,  0 ($sp)
    sw $t1,  4 ($sp)
    sw $t2,  8 ($sp)
    sw $t3, 12 ($sp)
    sw $t4, 16 ($sp)
    sw $t5, 20 ($sp)
    sw $t6, 24 ($sp)
    sw $t7, 28 ($sp)
    sw $t8, 32 ($sp)
    sw $t9, 36 ($sp)

    mflo $k0
    mfhi $k1

    sw $a0, 40 ($sp)
    sw $a1, 44 ($sp)
    sw $a2, 48 ($sp)
    sw $a3, 52 ($sp)

    sw $v0, 56 ($sp)
    sw $v1, 60 ($sp)

    /* Store LO/HI */
    sw $k0, 64 ($sp)
    sw $k1, 68 ($sp)

    sw $ra, 72 ($sp)

    /* Call the C routine. */
    jal interrupt_handler
     nop

    /* Restore registers. */

    lw $ra, 72 ($sp)

    lw $k0, 64 ($sp)
    lw $k1, 68 ($sp)
    mtlo $k0
    mthi $k1

    lw $v0, 56 ($sp)
    lw $v1, 60 ($sp)


    lw $a0, 40 ($sp)
    lw $a1, 44 ($sp)
    lw $a2, 48 ($sp)
    lw $a3, 52 ($sp)

    lw $t0,  0 ($sp)
    lw $t1,  4 ($sp)
    lw $t2,  8 ($sp)
    lw $t3, 12 ($sp)
    lw $t4, 16 ($sp)
    lw $t5, 20 ($sp)
    lw $t6, 24 ($sp)
    lw $t7, 28 ($sp)
    lw $t8, 32 ($sp)
    lw $t9, 36 ($sp)

    /* Free stack. */
    addu    $sp, $sp, REG_STACK_STORE_SIZE

    /* Re-enable interrupts. */
    ei
    /* Exception return. */
    eret
.Lasd:
    j .Lasd
     nop
