#include <asm.h>
#include <mips/m32c0.h>

        .set	noreorder       // Don't allow the assembler to reorder instructions.

        .section .ebase

        .org 0x0
        sdbbp
        nop

        .org 0x180
        sdbbp
        nop

        .org 0x200
        la      $k0, _intr_handler
        jr      $k0
        nop

        .org 0x280
        sdbbp
        nop

        .org 0x300
        sdbbp
        nop

        .org 0x380
        sdbbp
        nop

        .org 0x480
        sdbbp
        nop

        .set	noat            // Don't allow the assembler to use r1(at) for synthetic instr.

#define REG_STACK_STORE_SIZE 76

LEAF(_intr_handler)
        /* Please, no nested interrupts for now. */
        di

        /* Allocate space for register storage on the stack. */
        subu    $sp, $sp, REG_STACK_STORE_SIZE

        /*
         * Save registers on the stack.
         * Saving all t, a and v registers, as well as LO and HI.
         * k registers do not require saving.
         * s and sp registers are saved by compiler-generated
         * procedure entry, if the procedure modifies them.
         */
        sw      $t0, ($sp)
        sw      $t1, 4($sp)
        sw      $t2, 8($sp)
        sw      $t3, 12($sp)
        sw      $t4, 16($sp)
        sw      $t5, 20($sp)
        sw      $t6, 24($sp)
        sw      $t7, 28($sp)
        sw      $t8, 32($sp)
        sw      $t9, 36($sp)
        sw      $a0, 40($sp)
        sw      $a1, 44($sp)
        sw      $a2, 48($sp)
        sw      $a3, 52($sp)
        sw      $v0, 56($sp)
        sw      $v1, 60($sp)
        sw      $ra, 64($sp)

        mflo    $k0
        sw      $k0, 68($sp)
        mfhi    $k0
        sw      $k0, 72($sp)

        /* Call the C routine. */
        jal     intr_dispatcher
        nop

        /* Restore registers. */
        lw      $k0, 72($sp)
        mthi    $k0
        lw      $k0, 68($sp)
        mtlo    $k0

        lw      $ra, 64($sp)
        lw      $v1, 60($sp)
        lw      $v0, 56($sp)
        lw      $a3, 52($sp)
        lw      $a2, 48($sp)
        lw      $a1, 44($sp)
        lw      $a0, 40($sp)
        lw      $t9, 36($sp)
        lw      $t8, 32($sp)
        lw      $t7, 28($sp)
        lw      $t6, 24($sp)
        lw      $t5, 20($sp)
        lw      $t4, 16($sp)
        lw      $t3, 12($sp)
        lw      $t2, 8($sp)
        lw      $t1, 4($sp)
        lw      $t0, ($sp)

        /* Free stack frame */
        addu    $sp, $sp, REG_STACK_STORE_SIZE

        /* Re-enable interrupts. */
        ei

        eret

END(_intr_handler)
