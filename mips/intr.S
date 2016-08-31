#include <asm.h>
#include <mips/ctx.h>
#include <mips/m32c0.h>

#include "genassym.h"

        # Don't allow the assembler to reorder instructions.
        .set	noreorder

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
        mfc0    $k0, C0_CONTEXT         # Load UPT address which contains PTE
        sra     $k0, 1                  # PT is at the beginning of KSEG2
        lw      $k1, 0($k0)
        mtc0    $k1, C0_ENTRYLO0
        lw      $k1, 4($k0)
        mtc0    $k1, C0_ENTRYLO1
        ehb                             # clear hazard on tlbwr
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
        sll     $k0, $k0, 2             # Multiply exc_code by sizeof(void *)
        add     $k1, $k1, $k0           # Add exc_code
        lw      $k1, 0($k1)             # Load the address of a handler
        bnez    $k1, irq_handler        # If non-NULL then call irq_handler
        nop

1:      jal     kernel_oops
        nop

        # syscalls are not implemented yet
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

        # Forbid the assembler from using $at register.
        .set	noat

irq_handler:
        # Fetch the context from thread control block.
        mfc0    $k0, C0_USERLOCAL
        addi    $k0, TD_CONTEXT

        # Save all registers that are not preserved across calls...
        sw      $at, (REG_AT*4)($k0)
        sw      $a0, (REG_A0*4)($k0)
        sw      $a1, (REG_A1*4)($k0)
        sw      $a2, (REG_A2*4)($k0)
        sw      $a3, (REG_A3*4)($k0)
        sw      $v0, (REG_V0*4)($k0)
        sw      $v1, (REG_V1*4)($k0)
        sw      $t0, (REG_T0*4)($k0)
        sw      $t1, (REG_T1*4)($k0)
        sw      $t2, (REG_T2*4)($k0)
        sw      $t3, (REG_T3*4)($k0)
        sw      $t4, (REG_T4*4)($k0)
        sw      $t5, (REG_T5*4)($k0)
        sw      $t6, (REG_T6*4)($k0)
        sw      $t7, (REG_T7*4)($k0)
        sw      $t8, (REG_T8*4)($k0)
        sw      $t9, (REG_T9*4)($k0)
        mflo    $t0
        sw      $t0, (REG_LO*4)($k0)
        mfhi    $t0
        sw      $t0, (REG_HI*4)($k0)

        # ... return address,
        sw      $ra, (REG_RA*4)($k0)

        # ... and exception program counter.
        mfc0    $t0, C0_EPC
        sw      $t0, (REG_PC*4)($k0)

        # Call C interrupt handler routine.
        jalr    $k1
        nop

        # Fetch the context from thread control block.
        mfc0    $k0, C0_USERLOCAL
        addi    $k0, TD_CONTEXT

        # Restore exception program counter,
        lw      $k1, (REG_PC*4)($k0)
        mtc0    $k1, C0_EPC    

        # ... return address,
        lw      $ra, (REG_RA*4)($k0)

        # ... and all registers that are not preserved across calls.
        lw      $k1, (REG_HI*4)($k0)
        mthi    $k1
        lw      $k1, (REG_LO*4)($k0)
        mtlo    $k1
        lw      $t9, (REG_T9*4)($k0)
        lw      $t8, (REG_T8*4)($k0)
        lw      $t7, (REG_T7*4)($k0)
        lw      $t6, (REG_T6*4)($k0)
        lw      $t5, (REG_T5*4)($k0)
        lw      $t4, (REG_T4*4)($k0)
        lw      $t3, (REG_T3*4)($k0)
        lw      $t2, (REG_T2*4)($k0)
        lw      $t1, (REG_T1*4)($k0)
        lw      $t0, (REG_T0*4)($k0)
        lw      $v1, (REG_V1*4)($k0)
        lw      $v0, (REG_V0*4)($k0)
        lw      $a3, (REG_A3*4)($k0)
        lw      $a2, (REG_A2*4)($k0)
        lw      $a1, (REG_A1*4)($k0)
        lw      $a0, (REG_A0*4)($k0)
        lw      $at, (REG_AT*4)($k0)

        eret

# vim: sw=8 ts=8 et
