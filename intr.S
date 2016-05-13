#include <asm.h>
#include <mips/m32c0.h>

        .set	noreorder       // Don't allow the assembler to reorder instructions.

        .local tlb_refill
        .local cache_error
        .local general_exception
        .local irq_handler
        .local irq0
        .local irq1
        .local irq2
        .local irq3
        .local irq4
        .local irq5
        .local irq6
        .local irq7

        .section .ebase

        .org 0x0
tlb_refill:
        j tlb_exception_handler
        nop

        .org 0x100
cache_error:
1:      j       1b
        nop

        .org 0x180
general_exception:
        mfc0    $k0, C0_CAUSE
        andi    $k0, $k0, CR_X_MASK
        srl     $k0, $k0, CR_X_SHIFT
        li      $k1, EXC_SYS
        beq     $k1, $k0, 1f
        nop

        sll     $k0, $k0, 1            /* Multiply cause by two */
        la      $k1, ge_table          /* Compute address of proper handler in ge table */
        add     $k1, $k1, $k0
        jr      $k1                    /* Jump to handler */ 
        nop

        jal     kernel_oops
        nop

        /* syscalls not implemented yet */
1:      j       1b
        nop

        .org 0x200
irq0:
1:      j       1b
        nop

        .org 0x220
irq1:
1:      j       1b
        nop

        .org 0x240
irq2:
1:      j       1b
        nop

        .org 0x260
irq3:
1:      j       1b
        nop

        .org 0x280
irq4:
1:      j       1b
        nop

        .org 0x2a0
irq5:
1:      j       1b
        nop

        .org 0x2c0
irq6:
1:      j       1b
        nop

        .org 0x2e0
irq7:
        la      $k1,hardclock
        j       irq_handler
        nop

        .org 0x300

#define REG_STACK_STORE_SIZE 76

        .set	noat            // Don't allow the assembler to use r1(at) for synthetic instr.

irq_handler:
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
        jalr    $k1
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
/* Following is general exception table. General exception handler has 
 * very little space to use. So it jumps here, and from here we jump 
 * instantly to proper hander. All functions being jumped to, should 
 * have ((interrupt)) attribute, unless some exception is unhandled,
 * then these functions should panic the kernel. For exact meanings of
 * exception handlers numbers please check 5.23 Table of MIPS32 4KE User's Manual. */

ge_table:
        j       kernel_oops /* 0 */
        nop
        j       tlb_exception_handler /* 1 */
        nop
        j       tlb_exception_handler /* 2 */
        nop
        j       tlb_exception_handler /* 3 */
        nop
        j       kernel_oops /* 4 */
        nop
        j       kernel_oops /* 5 */
        nop
        j       kernel_oops /* 6 */
        nop
        j       kernel_oops /* 7 */
        nop
        j       kernel_oops /* 8 */
        nop
        j       kernel_oops /* 9 */
        nop
        j       kernel_oops /* 10 */
        nop
        j       kernel_oops /* 11 */
        nop
        j       kernel_oops /* 12 */
        nop
        j       kernel_oops /* 13 */
        nop
        nop           /* 14, reserved */
        nop
        nop           /* 15, reserved */
        nop
        j       kernel_oops /* 16 */
        nop
        j       kernel_oops /* 17 */
        nop
        j       kernel_oops /* 18 */
        nop
        nop           /* 19, reserved */
        nop
        nop           /* 20, reserved */
        nop
        nop           /* 21, reserved */
        nop
        nop           /* 22, reserved */
        nop
        nop           /* 23, reserved */
        nop
        j       kernel_oops /* 24, reserved */
        nop
        nop
