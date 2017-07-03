#include <mips/exc.h>
#include <mips/pcpu.h>

#include "assym.h"

        # Don't allow the assembler to reorder instructions.
        .set	noreorder
        # Forbid the assembler from using $at register.
        .set	noat

        .globl exc_enter
        .globl kern_exc_leave
        .globl user_exc_leave
        .local user_exc_enter
        .local kern_exc_enter

# [$k0] must be set to value of C0_STATUS
exc_enter:
        andi    k0, SR_KSU_MASK         # Did exception occur in kernel mode?
        beqz    k0, kern_exc_enter
        nop

user_exc_enter:
        # Fetch the context from thread control block.
        LOAD_PCPU(k0)
        lw      k0, PCPU_CURTHREAD(k0)
        addu    k0, TD_UCTX

        # Save all user registers...
        SAVE_REG(AT, AT, k0)
        SAVE_REG(v0, V0, k0)
        SAVE_REG(v1, V1, k0)
        SAVE_REG(a0, A0, k0)
        SAVE_REG(a1, A1, k0)
        SAVE_REG(a2, A2, k0)
        SAVE_REG(a3, A3, k0)
        SAVE_REG(t0, T0, k0)
        SAVE_REG(t1, T1, k0)
        SAVE_REG(t2, T2, k0)
        SAVE_REG(t3, T3, k0)
        SAVE_REG(t4, T4, k0)
        SAVE_REG(t5, T5, k0)
        SAVE_REG(t6, T6, k0)
        SAVE_REG(t7, T7, k0)
        SAVE_REG(s0, S0, k0)
        SAVE_REG(s1, S1, k0)
        SAVE_REG(s2, S2, k0)
        SAVE_REG(s3, S3, k0)
        SAVE_REG(s4, S4, k0)
        SAVE_REG(s5, S5, k0)
        SAVE_REG(s6, S6, k0)
        SAVE_REG(s7, S7, k0)
        SAVE_REG(t8, T8, k0)
        SAVE_REG(t9, T9, k0)
        SAVE_REG(gp, GP, k0)
        SAVE_REG(sp, SP, k0)
        SAVE_REG(fp, FP, k0)
        SAVE_REG(ra, RA, k0)
        mflo    t0
        mfhi    t1
        SAVE_REG(t0, LO, k0)
        SAVE_REG(t1, HI, k0)

        # ... and relevant coprocessor 0 registers.
        mfc0    t0, C0_STATUS
        mfc0    t1, C0_CAUSE
        mfc0    t2, C0_BADVADDR
        mfc0    t3, C0_EPC
        SAVE_REG(t0, SR, k0)
        SAVE_REG(t1, CAUSE, k0)
        SAVE_REG(t2, BADVADDR, k0)
        SAVE_REG(t3, PC, k0)

        # Load kernel global pointer.
        la      gp, _gp

        # Set kernel stack pointer to the bottom of kernel stack.
        #
        # We can enter user mode only with `user_exc_leave`. The routine does
        # not return and kernel context is not saved while entering user mode.
        # Hence we don't care about $sp and we can set it to kstack's bottom.
        LOAD_PCPU(t0)
        lw      t0, PCPU_CURTHREAD(t0)
        lw      sp, TD_KSTACK+STK_BASE(t0)
        lw      t1, TD_KSTACK+STK_SIZE(t0)
        add     sp, t1

        # No exeception frame so set td_frame to NULL.
        sw      $0, TD_KFRAME(t0)

        # Turn off FPU, enter kernel mode,
        # drop exception level and disable interrupts.
        mfc0    t0, C0_STATUS
        li      t1, ~(SR_CU1|SR_KSU_MASK|SR_EXL|SR_IE)
        and     t0, t1
        mtc0    t0, C0_STATUS

        # Call C interrupt handler routine.
        move    a0, k0
        jalr    k1
        nop

        # Interrupts may be enabled here and that's ok.
        LOAD_PCPU(t0)
        lw      t0, PCPU_CURTHREAD(t0)
        lw      t1, TD_FLAGS(t0)
        andi    t1, (TDF_NEEDSWITCH | TDF_NEEDSIGCHK)
        beqz    t1, user_exc_leave
        move    a0, sp
        jal     exc_before_leave
        nop

user_exc_leave:
        # Disable interrupts and fetch C0_STATUS.
        di      t0

        # Fetch the context from thread control block.
        LOAD_PCPU(k0)
        lw      k0, PCPU_CURTHREAD(k0)
        addu    k0, TD_UCTX

        # Update status register held in user context.
        LOAD_REG(t0, SR, k0)
        # Turn on FPU, enter exception level,
        # enter user mode and enable interrupts
        # (last two applies only after ERET).
        li      t1, (SR_CU1|SR_KSU_USER|SR_EXL|SR_IE)
        # Clear interrupts mask.
        li      t2, ~SR_IMASK
        or      t0, t1
        and     t0, t2
        # Update user-mode interrupts mask based on current one.
        mfc0    t1, C0_STATUS
        andi    t1, SR_IMASK
        or      t0, t1
        mtc0    t0, C0_STATUS
        SAVE_REG(t0, SR, k0)

        # Restore exception program counter,
        LOAD_REG(t0, PC, k0)
        mtc0    t0, C0_EPC   

        # ... all user registers.
        LOAD_REG(t0, HI, k0)
        LOAD_REG(t1, LO, k0)
        mthi    t0
        mtlo    t1
        LOAD_REG(ra, RA, k0)
        LOAD_REG(fp, FP, k0)
        LOAD_REG(gp, GP, k0)
        LOAD_REG(sp, SP, k0)
        LOAD_REG(t9, T9, k0)
        LOAD_REG(t8, T8, k0)
        LOAD_REG(s7, S7, k0)
        LOAD_REG(s6, S6, k0)
        LOAD_REG(s5, S5, k0)
        LOAD_REG(s4, S4, k0)
        LOAD_REG(s3, S3, k0)
        LOAD_REG(s2, S2, k0)
        LOAD_REG(s1, S1, k0)
        LOAD_REG(s0, S0, k0)
        LOAD_REG(t7, T7, k0)
        LOAD_REG(t6, T6, k0)
        LOAD_REG(t5, T5, k0)
        LOAD_REG(t4, T4, k0)
        LOAD_REG(t3, T3, k0)
        LOAD_REG(t2, T2, k0)
        LOAD_REG(t1, T1, k0)
        LOAD_REG(t0, T0, k0)
        LOAD_REG(a3, A3, k0)
        LOAD_REG(a2, A2, k0)
        LOAD_REG(a1, A1, k0)
        LOAD_REG(a0, A0, k0)
        LOAD_REG(v1, V1, k0)
        LOAD_REG(v0, V0, k0)
        LOAD_REG(AT, AT, k0)

        # This label is useful for debugging.
user_return:
        sync
        eret

kern_exc_enter:
        # Push stack frame.
        subu    sp, EXC_FRAME_SIZ

        # Save all user registers...
        SAVE_REG(AT, AT, sp)
        SAVE_REG(v0, V0, sp)
        SAVE_REG(v1, V1, sp)
        SAVE_REG(a0, A0, sp)
        SAVE_REG(a1, A1, sp)
        SAVE_REG(a2, A2, sp)
        SAVE_REG(a3, A3, sp)
        SAVE_REG(t0, T0, sp)
        SAVE_REG(t1, T1, sp)
        SAVE_REG(t2, T2, sp)
        SAVE_REG(t3, T3, sp)
        SAVE_REG(t4, T4, sp)
        SAVE_REG(t5, T5, sp)
        SAVE_REG(t6, T6, sp)
        SAVE_REG(t7, T7, sp)
        SAVE_REG(s0, S0, sp)
        SAVE_REG(s1, S1, sp)
        SAVE_REG(s2, S2, sp)
        SAVE_REG(s3, S3, sp)
        SAVE_REG(s4, S4, sp)
        SAVE_REG(s5, S5, sp)
        SAVE_REG(s6, S6, sp)
        SAVE_REG(s7, S7, sp)
        SAVE_REG(t8, T8, sp)
        SAVE_REG(t9, T9, sp)
        SAVE_REG(gp, GP, sp)
        add     t0, sp, EXC_FRAME_SIZ
        SAVE_REG(t0, SP, sp)
        SAVE_REG(fp, FP, sp)
        SAVE_REG(ra, RA, sp)
        mflo    t0
        mfhi    t1
        SAVE_REG(t0, LO, sp)
        SAVE_REG(t1, HI, sp)

        # ... and relevant coprocessor 0 registers.
        mfc0    t0, C0_STATUS
        mfc0    t1, C0_CAUSE
        mfc0    t2, C0_BADVADDR
        mfc0    t3, C0_EPC
        SAVE_REG(t0, SR, sp)
        SAVE_REG(t1, CAUSE, sp)
        SAVE_REG(t2, BADVADDR, sp)
        SAVE_REG(t3, PC, sp)

        # Drop exception level and disable interrupts.
        li      t1, ~(SR_EXL|SR_IE)
        and     t0, t1
        mtc0    t0, C0_STATUS

        # Load kernel global pointer.
        la      gp, _gp

        # Save stack frame pointer into td_frame.
        LOAD_PCPU(t0)
        lw      t0, PCPU_CURTHREAD(t0)
        sw      sp, TD_KFRAME(t0)

        # Call C interrupt handler routine.
        move    a0, sp
        jalr    k1
        nop

        # Interrupts may be enabled here and that's ok.
        LOAD_PCPU(t0)
        lw      t0, PCPU_CURTHREAD(t0)
        lw      t1, TD_FLAGS(t0)
        andi    t1, (TDF_NEEDSWITCH | TDF_NEEDSIGCHK)
        beqz    t1, kern_exc_leave
        move    a0, sp
        jal     exc_before_leave
        nop

kern_exc_leave:
        # Enter exception level (disable interrupts till ERET).
        mfc0    t0, C0_STATUS
        ori     t0, SR_EXL
        mtc0    t0, C0_STATUS

        # Update status register held in exception frame (only interrupt mask).
        LOAD_REG(t0, SR, sp)
        li      t1, ~SR_IMASK
        and     t0, t1
        mfc0    t1, C0_STATUS
        andi    t1, SR_IMASK
        or      t0, t1
        SAVE_REG(t0, SR, sp)

        # Restore exception program counter,
        LOAD_REG(t0, PC, sp)
        mtc0    t0, C0_EPC   

        # ... all user registers,
        LOAD_REG(t0, HI, sp)
        LOAD_REG(t1, LO, sp)
        mthi    t0
        mtlo    t1
        LOAD_REG(ra, RA, sp)
        LOAD_REG(fp, FP, sp)
        LOAD_REG(gp, GP, sp)
        LOAD_REG(k0, SP, sp)            # restore $sp later...
        LOAD_REG(t9, T9, sp)
        LOAD_REG(t8, T8, sp)
        LOAD_REG(s7, S7, sp)
        LOAD_REG(s6, S6, sp)
        LOAD_REG(s5, S5, sp)
        LOAD_REG(s4, S4, sp)
        LOAD_REG(s3, S3, sp)
        LOAD_REG(s2, S2, sp)
        LOAD_REG(s1, S1, sp)
        LOAD_REG(s0, S0, sp)
        LOAD_REG(t7, T7, sp)
        LOAD_REG(t6, T6, sp)
        LOAD_REG(t5, T5, sp)
        LOAD_REG(t4, T4, sp)
        LOAD_REG(t3, T3, sp)
        LOAD_REG(t2, T2, sp)
        LOAD_REG(t1, T1, sp)
        LOAD_REG(t0, T0, sp)
        LOAD_REG(a3, A3, sp)
        LOAD_REG(a2, A2, sp)
        LOAD_REG(a1, A1, sp)
        LOAD_REG(a0, A0, sp)
        LOAD_REG(v1, V1, sp)
        LOAD_REG(v0, V0, sp)
        LOAD_REG(AT, AT, sp)

        # ... and finally status register (assume EXL bit is set)
        LOAD_REG(k1, SR, sp)
        mtc0    k1, C0_STATUS

        # Restore stack frame.
        move    sp, k0

        sync
        eret

# vim: sw=8 ts=8 et
