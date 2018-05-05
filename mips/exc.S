#include <mips/exc.h>
#include <mips/pcpu.h>
#include <mips/regdef.h>

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
        .local skip_fpu_save
        .local skip_fpu_restore

# [$k0] must be set to value of C0_STATUS
exc_enter:
        andi    k0, SR_KSU_MASK         # Did exception occur in kernel mode?
        beqz    k0, kern_exc_enter
        nop

user_exc_enter:
        # Fetch the context from thread control block.
        LOAD_PCPU(k0)
        lw      k0, PCPU_CURTHREAD(k0)
        lw      k0, TD_UFRAME(k0)

        SAVE_CPU_CTX(k0)

        # If FPU is enabled save FPU registers.
        mfc0    t0, C0_STATUS
        ext     t1, t0, SR_CU1_SHIFT, 1
        beqz    t1, skip_fpu_save
        nop

        SAVE_FPU_CTX(k0)

skip_fpu_save:
        # Set kernel stack pointer just after saved user context.
        move    sp, k0

        # Fetch thread control block to s0 for later use.
        LOAD_PCPU(k0)
        lw      s0, PCPU_CURTHREAD(k0)

        # No exeception frame so set td_kframe to NULL.
        sw      $0, TD_KFRAME(s0)

        # Load kernel global pointer.
        la      gp, _gp

        # Increment interrupt nest level.
        lw      t0, TD_IDNEST(s0)
        addi    t0, 1
        sw      t0, TD_IDNEST(s0)

        # Turn off FPU, enter kernel mode,
        # drop exception level and disable interrupts.
        mfc0    t0, C0_STATUS
        li      t1, ~(SR_CU1|SR_KSU_MASK|SR_EXL|SR_IE)
        and     t0, t1
        mtc0    t0, C0_STATUS

        # Call C interrupt handler routine.
        move    a0, sp
        jalr    k1
        nop

        # Decrement interrupt nest level.
        lw      t0, TD_IDNEST(s0)
        addi    t0, -1
        sw      t0, TD_IDNEST(s0)

user_exc_leave:
        # Disable interrupts saving current SR into t1.
        di      t1

        # Fetch the context from thread control block.
        LOAD_PCPU(k1)
        lw      k1, PCPU_CURTHREAD(k1)
        lw      k0, TD_UFRAME(k1)

        # Update status register held in user context.
        LOAD_REG(t0, SR, k0)
        # Enter exception level, enter user mode and enable interrupts
        # (last two applies only after ERET).
        ori     t0, (SR_KSU_USER|SR_EXL|SR_IE)
        # Update user-mode interrupts mask based on current one.
        ext     t1, t1, SR_IMASK_SHIFT, SR_IMASK_BITS
        ins     t0, t1, SR_IMASK_SHIFT, SR_IMASK_BITS
        mtc0    t0, C0_STATUS
        SAVE_REG(t0, SR, k0)

        # If FPU has been enabled, then restore FPU registers.
        ext     t1, t0, SR_CU1_SHIFT, 1
        beqz    t1, skip_fpu_restore
        nop

        LOAD_FPU_CTX(k0)

skip_fpu_restore:
        LOAD_CPU_CTX(k0)

        # This label is useful for debugging.
user_return:
        sync
        eret

kern_exc_enter:
        # Allocate stack frame and save context there.
        subu    k0, sp, EXC_FRAME_SIZ
        SAVE_CPU_CTX(k0)

        # Set kernel stack pointer just after saved kernel context.
        move    sp, k0

        # Drop exception level and disable interrupts.
        li      t1, ~(SR_EXL|SR_IE)
        and     t0, t1
        mtc0    t0, C0_STATUS

        # Load kernel global pointer.
        la      gp, _gp

        # Save stack frame pointer into td_kframe.
        LOAD_PCPU(t0)
        lw      s0, PCPU_CURTHREAD(t0)          # [s0] curthread
        sw      sp, TD_KFRAME(s0)

        # Increment interrupt nest level.
        lw      t0, TD_IDNEST(s0)
        addi    t0, 1
        sw      t0, TD_IDNEST(s0)

        # Call C interrupt handler routine.
        move    a0, sp
        jalr    k1
        nop

        # Decrement interrupt nest level.
        lw      t0, TD_IDNEST(s0)
        addi    t0, -1
        sw      t0, TD_IDNEST(s0)

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

        # Load context from exception frame on stack, sp will get overwritten,
        move    k0, sp
        LOAD_CPU_CTX(k0)

        # ... and finally status register (assume EXL bit is set)
        LOAD_REG(k1, SR, k0)
        mtc0    k1, C0_STATUS

        sync
        eret

# vim: sw=8 ts=8 et
