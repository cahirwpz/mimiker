#define _WIREDCODE
#include <mips/exc.h>
#include <mips/pcpu.h>
#include <mips/regdef.h>
#include <mips/vm_param.h>

#define STACKSIZE PAGESIZE

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
        mfc0    k0, C0_STATUS           
        andi    k0, SR_KSU_MASK         # Did exception occur in kernel mode?
        beqz    k0, kern_exc_enter
        nop

# IMPORTANT: While exception level is active the code may only access kernel
# stack and local pcpu structure! FTTB These are accessed through kseg0.
# It is UNSAFE to use k0/k1 registers when TLB miss handler may be triggered!

NESTED(user_exc_enter, USER_EXC_FRAME_SIZE, ra)
        .cfi_signal_frame

        # Move user sp to temporary register and replace it with kernel sp.
        move    k1, sp
        # Previous value of sp is saved in register k1.
        .cfi_register sp, k1

        # Exception frame will be saved on kernel stack. 
        # Fetch exception frame pointer where context will be saved.
        LOAD_PCPU_KSEG0(k0)

        # sp contains kernel stack pointer.
        lw      sp, PCPU_KSP(k0)

        # Define a rule for computing CFA as: *sp + 0.
        .cfi_def_cfa sp, 0
        SAVE_CPU_CTX(k1, sp)

        # If FPU is enabled save FPU registers.
        mfc0    t0, C0_STATUS
        ext     t1, t0, SR_CU1_SHIFT, 1
        beqz    t1, skip_fpu_save
        nop

        SAVE_FPU_CTX(sp)

skip_fpu_save:
        # Load kernel global pointer.
        la      gp, _gp

        # Turn off FPU, enter kernel mode,
        # drop exception level and disable interrupts.
        mfc0    t0, C0_STATUS
        li      t1, ~(SR_CU1|SR_KSU_MASK|SR_EXL|SR_IE)
        and     t0, t1
        mtc0    t0, C0_STATUS

        # Fetch thread control block pointer to s0 for later use.
        LOAD_PCPU(s0)
        lw      s0, PCPU_CURTHREAD(s0)

        # No exeception frame so set td_kframe to NULL.
        sw      $0, TD_KFRAME(s0)

        # Increment interrupt nest level.
        lw      t0, TD_IDNEST(s0)
        addi    t0, 1
        sw      t0, TD_IDNEST(s0)

        # Call C interrupt handler routine.
        jal     mips_exc_handler
        move    a0, sp                  # (delay) 1st arg

        # Decrement interrupt nest level.
        lw      t0, TD_IDNEST(s0)
        addi    t0, -1
        sw      t0, TD_IDNEST(s0)

user_exc_leave:
        # Disable interrupts and extract interrupt mask into t1.
        di      t1
        ehb
        ext     t1, t1, SR_IMASK_SHIFT, SR_IMASK_BITS

        # Set current stack pointer to user exception frame.
        # This is crucial on first entry to user-space for this thread.
        LOAD_PCPU(s0)
        lw      t0, PCPU_CURTHREAD(s0)
        lw      sp, TD_UFRAME(t0)
        
        # Update kernel stack pointer to be used on kernel reentry.
        sw      sp, PCPU_KSP(s0)

        # Update status register held in exception frame (only interrupt mask).
        LOAD_REG(t0, SR, sp)
        ins     t0, t1, SR_IMASK_SHIFT, SR_IMASK_BITS
        SAVE_REG(t0, SR, sp)

        # Enter exception level with user-mode settings.
        ori     t0, SR_EXL
        mtc0    t0, C0_STATUS

        # If FPU has been enabled, then restore FPU registers.
        ext     t1, t0, SR_CU1_SHIFT, 1
        beqz    t1, skip_fpu_restore
        nop

        LOAD_FPU_CTX(sp)

skip_fpu_restore:
        # Load context from exception frame on stack, sp will get overwritten.
        move    k0, sp
        LOAD_CPU_CTX(k0)

        # This label is useful for debugging.
        sync
user_return:
        eret
END(user_exc_enter)

NESTED(kern_exc_enter, KERN_EXC_FRAME_SIZE, ra)
        # Allocate stack frame (of KERN_EXC_FRAME_SIZE bytes) and save context.
        move    k0, sp                  # save original sp value
        subu    sp, KERN_EXC_FRAME_SIZE # required by gdb to recognize frame
        SAVE_CPU_CTX(k0, sp)

        # In normal conditions we're going to enter regular exception handler.
        la      s1, mips_exc_handler

        # If there's not enough space on the stack to store another exception
        # frame we consider situation to be critical and panic.
        andi    t0, sp, STACKSIZE - 1
        subu    t0, KERN_EXC_FRAME_SIZE
        bgez    t0, 1f
        nop
        # Hopefully KERN_EXC_FRAME_SIZE bytes of unallocated stack space will be
        # enough to enter kernel stack overflow handler.
        la      s1, kstack_overflow_handler

        # Load kernel global pointer.
1:      la      gp, _gp

        # Drop exception level and disable interrupts.
        mfc0    t0, C0_STATUS
        li      t1, ~(SR_EXL|SR_IE)
        and     t0, t1
        mtc0    t0, C0_STATUS

        # Fetch thread control block pointer to s0 for later use.
        LOAD_PCPU(t0)
        lw      s0, PCPU_CURTHREAD(t0)

        # Save exception frame pointer into td_kframe.
        sw      sp, TD_KFRAME(s0)

        # Increment interrupt nest level.
        lw      t0, TD_IDNEST(s0)
        addi    t0, 1
        sw      t0, TD_IDNEST(s0)

        # Call C interrupt handler routine.
        jalr    s1
        move    a0, sp                  # (delay) 1st arg

        # Decrement interrupt nest level.
        lw      t0, TD_IDNEST(s0)
        addi    t0, -1
        sw      t0, TD_IDNEST(s0)

kern_exc_leave:
        # Disable interrupts and extract interrupt mask into t1.
        di      t1
        ehb
        ext     t1, t1, SR_IMASK_SHIFT, SR_IMASK_BITS

        # Load status register from exception frame and update it with current
        # interrupt mask.
        LOAD_REG(t0, SR, sp)
        ins     t0, t1, SR_IMASK_SHIFT, SR_IMASK_BITS

        # Enter exception level with kernel-mode settings.
        ori     t0, SR_EXL
        mtc0    t0, C0_STATUS

        # Load context from exception frame on stack, sp will get overwritten.
        move    k0, sp
        LOAD_CPU_CTX(k0)

        sync
        eret
END(kern_exc_enter)

# vim: sw=8 ts=8 et
