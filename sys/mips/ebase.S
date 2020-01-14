#include <mips/exception.h>
#include <mips/m32c0.h>
#include <mips/pcpu.h>
#include <mips/regdef.h>
#include <mips/pmap.h>
#include <mips/vm_param.h>

#define STACKSIZE PAGESIZE

#include "assym.h"

        # Don't allow the assembler to reorder instructions.
        .set	noreorder
        # Forbid the assembler from using $at register.
        .set	noat

        .global _ebase

        .section .ebase, "ax"

_ebase:

        .org    0x0

# TLB miss handler must avoid generating TLB miss during PT search.
# 
# If you use TLBR instruction remember that it overwrites EntryHi!
#
# Please verify EHB (clear execution hazard) instruction placement against
# chapter 2.8 of "MIPS32® 24KETM Processor Core Family Software User’s Manual".
# EHB has to be put between a producer and a consumer - preferably just before
# the consumer, not just after the producer.

SLEAF(tlb_refill)
        # Read PDE associated with bad virtual address.
        # Highest bit of the address switches between UPD_BASE & KPD_BASE,
        # so it's copied into 12th position with arithmetic shift.
        mfc0    k1, C0_BADVADDR
        sra     k1, 20
        andi    k1, 0x1ffc
        la      k0, UPD_BASE
        addu    k0, k1
        lw      k1, (k0)                # [k1] PDE of bad virtual address

        # If PDE isn't valid behave as if TLBL/TLBS exception was triggered.
        andi    k0, k1, ENTRYLO0_V_MASK
        beqz    k0, exc_enter
        nop     

        # Calculate page table address from PDE
        srl     k1, 6
        sll     k1, 12
        # ... and index of even entry corresponding to bad virtual address.
        mfc0    k0, C0_BADVADDR
        srl     k0, 10
        andi    k0, 0xff8
        addu    k0, k1

        # Simply grab the PTEs and load them into the TLB.
        lw      k1, 0(k0)
        mtc0    k1, C0_ENTRYLO0
        lw      k1, 4(k0)
        mtc0    k1, C0_ENTRYLO1
        ehb
        tlbwr
        eret
END(tlb_refill)

        .org    0x100
        .local  cache_error

SLEAF(cache_error)
1:      j       1b
        nop
END(cache_error)

        .org    0x180
        .local  general_exception

SLEAF(general_exception)
        j       exc_enter
        nop
END(general_exception)

        .org    0x200
        .local  irq

SLEAF(irq)
        j       exc_enter
        nop
END(irq)

        .org    0x500
        .global user_exc_leave
        .global kern_exc_leave

# [$k0] must be set to value of C0_STATUS
exc_enter:
        mfc0    k0, C0_STATUS           
        andi    k0, SR_KSU_MASK         # Did exception occur in kernel mode?
        beqz    k0, kern_exc_enter
        nop

# IMPORTANT: While exception level is active the code may only access kernel
# stack and local pcpu structure! FTTB These are accessed through kseg0.
# It is UNSAFE to use k0/k1 registers when TLB miss handler may be triggered!

SNESTED(user_exc_enter, USER_EXC_FRAME_SIZE, ra)
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
        la      t0, mips_exc_handler
        jalr    t0
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

SNESTED(kern_exc_enter, KERN_EXC_FRAME_SIZE, ra)
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
