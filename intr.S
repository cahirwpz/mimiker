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
        mfc0 $k0, C0_CONTEXT        # Load UPT address which contains PTE
        sra $k0,1                   # PT should be located in KSEG2
        lw  $k1, 0($k0)
        mtc0 $k1, C0_ENTRYLO0
        lw  $k1, 4($k0)
        mtc0 $k1, C0_ENTRYLO1
        ehb                         # mtc0, Hazard on tlbwi
        tlbwr
        eret

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
        beq     $k1, $k0, 2f
        nop

        la      $k1, general_exception_table
        sll     $k0, $k0, 2   /* Multiply exc_code by sizeof(void *) */  
        add     $k1, $k1, $k0 /* Add exc_code */  
        lw      $k1, 0($k1)   /* Load the address of a handler */
        beqz    $k1, 1f       /* If NULL then call kernel_oops */
        nop
        jr      $k1           /* Jump to handler */ 
        nop

1:      jal     kernel_oops
        nop

        /* syscalls not implemented yet */
2:      j       2b
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

#define REG_STACK_STORE_SIZE 84

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

        mfc0    $k0, $14
        sw      $k0, 76($sp)


        /* Call the C routine. */
        jalr    $k1
        nop

        /* Restore registers. */
        lw      $k0, 76($sp)
        mtc0    $k0, $14    

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
