#include <mips/ctx.h>
#include <mips/m32c0.h>

#include "genassym.h"

        # Don't allow the assembler to reorder instructions.
        .set	noreorder
        # Forbid the assembler from using $at register.
        .set	noat

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
        mfc0    $k0, C0_CONTEXT         # Load PT address which contains PTEs
        mfc0    $k1, C0_BADVADDR
        andi    $k1, 0x1000
        bnez    $k1, 2f
        sra     $k0, 1                  # PT is at the beginning of KSEG2

        # Both cases can fail if corresponding PDE is not in TLB.
        # We want to fail first on address that refer to the faulted page,
        # so we can recover correct address in tlb_exception_handler.
1:      lw      $k1, 0($k0)
        mtc0    $k1, C0_ENTRYLO0
        lw      $k1, 4($k0)
        mtc0    $k1, C0_ENTRYLO1
        ehb                             # clear hazard on tlbwr
        tlbwr
        eret

2:      lw      $k1, 4($k0)
        mtc0    $k1, C0_ENTRYLO1
        lw      $k1, 0($k0)
        mtc0    $k1, C0_ENTRYLO0
        ehb                             # clear hazard on tlbwr
        tlbwr
        eret

        .org 0x100
cache_error:
1:      j       1b
        nop

        .org 0x180
general_exception:
        mfc0    $k1, C0_CAUSE
        andi    $k1, CR_X_MASK
        la      $k0, general_exception_table
        add     $k1, $k0
        lw      $k1, 0($k1)             # Load the address of a handler
        bnez    $k1, exc_enter          # If non-NULL then enter handler
        mfc0    $k0, C0_STATUS          # (delay slot) load status register
        la      $k1, kernel_oops        # If no handler then panic!

exc_enter:
        andi    $k0, SR_KSU_MASK        # Did exception occur in kernel mode?
        beqz    $k0, kernel_exc_enter
        nop
        j       user_exc_enter
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
        j       exc_enter
        mfc0    $k0, C0_STATUS          # (delay slot) load status register

        .org 0x300

user_exc_enter:
        # Fetch the context from thread control block.
        mfc0    $k0, C0_USERLOCAL
        addu    $k0, TD_USERCTX

        # Save all user registers...
        SAVE_REG($at, AT, $k0)
        SAVE_REG($v0, V0, $k0)
        SAVE_REG($v1, V1, $k0)
        SAVE_REG($a0, A0, $k0)
        SAVE_REG($a1, A1, $k0)
        SAVE_REG($a2, A2, $k0)
        SAVE_REG($a3, A3, $k0)
        SAVE_REG($t0, T0, $k0)
        SAVE_REG($t1, T1, $k0)
        SAVE_REG($t2, T2, $k0)
        SAVE_REG($t3, T3, $k0)
        SAVE_REG($t4, T4, $k0)
        SAVE_REG($t5, T5, $k0)
        SAVE_REG($t6, T6, $k0)
        SAVE_REG($t7, T7, $k0)
        SAVE_REG($s0, S0, $k0)
        SAVE_REG($s1, S1, $k0)
        SAVE_REG($s2, S2, $k0)
        SAVE_REG($s3, S3, $k0)
        SAVE_REG($s4, S4, $k0)
        SAVE_REG($s5, S5, $k0)
        SAVE_REG($s6, S6, $k0)
        SAVE_REG($s7, S7, $k0)
        SAVE_REG($t8, T8, $k0)
        SAVE_REG($t9, T9, $k0)
        SAVE_REG($gp, GP, $k0)
        SAVE_REG($sp, SP, $k0)
        SAVE_REG($fp, FP, $k0)
        SAVE_REG($ra, RA, $k0)
        mflo    $t0
        mfhi    $t1
        SAVE_REG($t0, LO, $k0)
        SAVE_REG($t1, HI, $k0)

        # ... and relevant coprocessor 0 registers.
        mfc0    $t0, C0_STATUS
        mfc0    $t1, C0_CAUSE
        mfc0    $t2, C0_BADVADDR
        mfc0    $t3, C0_EPC
        SAVE_REG($t0, SR, $k0)
        SAVE_REG($t1, CAUSE, $k0)
        SAVE_REG($t2, BADVADDR, $k0)
        SAVE_REG($t3, PC, $k0)

        # TODO: switch to kernel SP!
        la      $gp, _gp

        # Turn off FPU, enter kernel mode,
        # drop exception level and disable interrupts.
        li      $t1, ~(SR_CU1|SR_KSU_KERN|SR_EXL|SR_IE)
        and     $t0, $t1
        mtc0    $t0, C0_STATUS

        # Call C interrupt handler routine.
        jalr    $k1
        nop

        # Disable interrupts till ERET.
        di

        # Fetch the context from thread control block.
        mfc0    $k0, C0_USERLOCAL
        addu    $k0, TD_USERCTX

        # Update status register held in user context (interrupt mask).
        LOAD_REG($t0, SR, $k0)
        li      $t1, ~SR_IMASK
        and     $t0, $t1
        mfc0    $t1, C0_STATUS
        andi    $t1, SR_IMASK
        or      $t0, $t1
        SAVE_REG($t0, SR, $k0)

        # Restore exception program counter,
        LOAD_REG($t0, PC, $k0)
        mtc0    $t0, C0_EPC   

        # ... all user registers,
        LOAD_REG($t0, HI, $k0)
        LOAD_REG($t1, LO, $k0)
        mthi    $t0
        mtlo    $t1
        LOAD_REG($ra, RA, $k0)
        LOAD_REG($fp, FP, $k0)
        LOAD_REG($gp, GP, $k0)
        LOAD_REG($sp, SP, $k0)
        LOAD_REG($t9, T9, $k0)
        LOAD_REG($t8, T8, $k0)
        LOAD_REG($s7, S7, $k0)
        LOAD_REG($s6, S6, $k0)
        LOAD_REG($s5, S5, $k0)
        LOAD_REG($s4, S4, $k0)
        LOAD_REG($s3, S3, $k0)
        LOAD_REG($s2, S2, $k0)
        LOAD_REG($s1, S1, $k0)
        LOAD_REG($s0, S0, $k0)
        LOAD_REG($t7, T7, $k0)
        LOAD_REG($t6, T6, $k0)
        LOAD_REG($t5, T5, $k0)
        LOAD_REG($t4, T4, $k0)
        LOAD_REG($t3, T3, $k0)
        LOAD_REG($t2, T2, $k0)
        LOAD_REG($t1, T1, $k0)
        LOAD_REG($t0, T0, $k0)
        LOAD_REG($a3, A3, $k0)
        LOAD_REG($a2, A2, $k0)
        LOAD_REG($a1, A1, $k0)
        LOAD_REG($a0, A0, $k0)
        LOAD_REG($v1, V1, $k0)
        LOAD_REG($v0, V0, $k0)
        LOAD_REG($at, AT, $k0)

        # ... and finally status register.
        LOAD_REG($k1, SR, $k0)
        mtc0    $k1, C0_STATUS

        sync
        eret

kernel_exc_enter:
        # Push stack frame.
        subu    $sp, EXC_FRAME_SIZE

        # Save all user registers...
        SAVE_REG($at, AT, $sp)
        SAVE_REG($v0, V0, $sp)
        SAVE_REG($v1, V1, $sp)
        SAVE_REG($a0, A0, $sp)
        SAVE_REG($a1, A1, $sp)
        SAVE_REG($a2, A2, $sp)
        SAVE_REG($a3, A3, $sp)
        SAVE_REG($t0, T0, $sp)
        SAVE_REG($t1, T1, $sp)
        SAVE_REG($t2, T2, $sp)
        SAVE_REG($t3, T3, $sp)
        SAVE_REG($t4, T4, $sp)
        SAVE_REG($t5, T5, $sp)
        SAVE_REG($t6, T6, $sp)
        SAVE_REG($t7, T7, $sp)
        SAVE_REG($s0, S0, $sp)
        SAVE_REG($s1, S1, $sp)
        SAVE_REG($s2, S2, $sp)
        SAVE_REG($s3, S3, $sp)
        SAVE_REG($s4, S4, $sp)
        SAVE_REG($s5, S5, $sp)
        SAVE_REG($s6, S6, $sp)
        SAVE_REG($s7, S7, $sp)
        SAVE_REG($t8, T8, $sp)
        SAVE_REG($t9, T9, $sp)
        SAVE_REG($gp, GP, $sp)
        add     $t0, $sp, EXC_FRAME_SIZE
        SAVE_REG($t0, SP, $sp)
        SAVE_REG($fp, FP, $sp)
        SAVE_REG($ra, RA, $sp)
        mflo    $t0
        mfhi    $t1
        SAVE_REG($t0, LO, $sp)
        SAVE_REG($t1, HI, $sp)

        # ... and relevant coprocessor 0 registers.
        mfc0    $t0, C0_STATUS
        mfc0    $t1, C0_CAUSE
        mfc0    $t2, C0_BADVADDR
        mfc0    $t3, C0_EPC
        SAVE_REG($t0, SR, $sp)
        SAVE_REG($t1, CAUSE, $sp)
        SAVE_REG($t2, BADVADDR, $sp)
        SAVE_REG($t3, PC, $sp)

        # Drop exception level and disable interrupts.
        li      $t1, ~(SR_EXL|SR_IE)
        and     $t0, $t1
        mtc0    $t0, C0_STATUS

        # Load kernel global pointer.
        la      $gp, _gp

        # Save stack frame pointer into td_frame.
        mfc0    $t0, C0_USERLOCAL
        sw      $sp, TD_FRAME($t0)

        # Call C interrupt handler routine.
        jalr    $k1
        nop

        # Disable interrupts till ERET.
        di

        # Update status register held in user context (interrupt mask).
        LOAD_REG($t0, SR, $sp)
        li      $t1, ~SR_IMASK
        and     $t0, $t1
        mfc0    $t1, C0_STATUS
        andi    $t1, SR_IMASK
        or      $t0, $t1
        SAVE_REG($t0, SR, $sp)

        # Restore exception program counter,
        LOAD_REG($t0, PC, $sp)
        mtc0    $t0, C0_EPC   

        # ... all user registers,
        LOAD_REG($t0, HI, $sp)
        LOAD_REG($t1, LO, $sp)
        mthi    $t0
        mtlo    $t1
        LOAD_REG($ra, RA, $sp)
        LOAD_REG($fp, FP, $sp)
        LOAD_REG($gp, GP, $sp)
        LOAD_REG($k0, SP, $sp)          # restore $sp later...
        LOAD_REG($t9, T9, $sp)
        LOAD_REG($t8, T8, $sp)
        LOAD_REG($s7, S7, $sp)
        LOAD_REG($s6, S6, $sp)
        LOAD_REG($s5, S5, $sp)
        LOAD_REG($s4, S4, $sp)
        LOAD_REG($s3, S3, $sp)
        LOAD_REG($s2, S2, $sp)
        LOAD_REG($s1, S1, $sp)
        LOAD_REG($s0, S0, $sp)
        LOAD_REG($t7, T7, $sp)
        LOAD_REG($t6, T6, $sp)
        LOAD_REG($t5, T5, $sp)
        LOAD_REG($t4, T4, $sp)
        LOAD_REG($t3, T3, $sp)
        LOAD_REG($t2, T2, $sp)
        LOAD_REG($t1, T1, $sp)
        LOAD_REG($t0, T0, $sp)
        LOAD_REG($a3, A3, $sp)
        LOAD_REG($a2, A2, $sp)
        LOAD_REG($a1, A1, $sp)
        LOAD_REG($a0, A0, $sp)
        LOAD_REG($v1, V1, $sp)
        LOAD_REG($v0, V0, $sp)
        LOAD_REG($at, AT, $sp)

        # ... and finally status register.
        LOAD_REG($k1, SR, $sp)
        mtc0    $k1, C0_STATUS

        # Restore stack frame.
        move    $sp, $k0

        sync
        eret

# vim: sw=8 ts=8 et
