#include <asm.h>
#include <asm-mips.h>
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

        .globl irq_return

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

        /* TEMPORARY. Will change it if this approach is accepted. */
        beq $k0, 13, irq_yield
        nop
        /* END OF TEMPORARY */

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
        /* Please, no nested interrupts for now. */
        di

        la      $k1,hardclock
        j       irq_handler
        nop

        .org 0x300

irq_yield:
        mfc0    $k0, C0_EPC
        addi    $k0, $k0, 4
        mtc0    $k0, C0_EPC

        la      $k1, sched_switch
        j       irq_handler
        nop

        .set	noat      /* Forbid the assembler from using $at register. */

irq_handler:
        /* Allocate space for register storage on the stack. */
        subu    $sp, $sp, REG_NUM * 4

        /* Save all registers that are not preserved across calls... */
        sw      $at, (REG_AT*4)($sp)
        sw      $a0, (REG_A0*4)($sp)
        sw      $a1, (REG_A1*4)($sp)
        sw      $a2, (REG_A2*4)($sp)
        sw      $a3, (REG_A3*4)($sp)
        sw      $v0, (REG_V0*4)($sp)
        sw      $v1, (REG_V1*4)($sp)
        sw      $t0, (REG_T0*4)($sp)
        sw      $t1, (REG_T1*4)($sp)
        sw      $t2, (REG_T2*4)($sp)
        sw      $t3, (REG_T3*4)($sp)
        sw      $t4, (REG_T4*4)($sp)
        sw      $t5, (REG_T5*4)($sp)
        sw      $t6, (REG_T6*4)($sp)
        sw      $t7, (REG_T7*4)($sp)
        sw      $t8, (REG_T8*4)($sp)
        sw      $t9, (REG_T9*4)($sp)
        mflo    $k0
        sw      $k0, (REG_LO*4)($sp)
        mfhi    $k0
        sw      $k0, (REG_HI*4)($sp)

        /* ... return address, */
        sw      $ra, (REG_RA*4)($sp)

        /* ... and exception program counter. */
        mfc0    $k0, C0_EPC
        sw      $k0, (REG_EPC*4)($sp)

        /* Call the C routine. */
        jalr    $k1
        nop

irq_return:
        /* Restore exception program counter... */
        lw      $k0, (REG_EPC*4)($sp)
        mtc0    $k0, C0_EPC    

        /* ... return address, */
        lw      $ra, (REG_RA*4)($sp)

        /* ... and all registers that are not preserved across calls. */
        lw      $k0, (REG_HI*4)($sp)
        mthi    $k0
        lw      $k0, (REG_LO*4)($sp)
        mtlo    $k0
        lw      $t9, (REG_T9*4)($sp)
        lw      $t8, (REG_T8*4)($sp)
        lw      $t7, (REG_T7*4)($sp)
        lw      $t6, (REG_T6*4)($sp)
        lw      $t5, (REG_T5*4)($sp)
        lw      $t4, (REG_T4*4)($sp)
        lw      $t3, (REG_T3*4)($sp)
        lw      $t2, (REG_T2*4)($sp)
        lw      $t1, (REG_T1*4)($sp)
        lw      $t0, (REG_T0*4)($sp)
        lw      $v1, (REG_V1*4)($sp)
        lw      $v0, (REG_V0*4)($sp)
        lw      $a3, (REG_A3*4)($sp)
        lw      $a2, (REG_A2*4)($sp)
        lw      $a1, (REG_A1*4)($sp)
        lw      $a0, (REG_A0*4)($sp)
        lw      $at, (REG_AT*4)($sp)

        /* Free stack frame. */
        addu    $sp, $sp, REG_NUM * 4

        /* Re-enable interrupts. */
        ei

        eret
