#include <mips/asm.h>
#include <mips/pcpu.h>
#include <mips/regdef.h>
#include <mips/m32c0.h>

#include "assym.h"

        .set noreorder

# Boot environment is described in MIPS® YAMON™ User's Manual, Chapter 5
# ... and MIPS® YAMON™ Reference Manual, Chapter 8.3
#
# $a0 = argc
# $a1 = argv
# $a2 = envp
# $a3 = memsize
#
# $sp is initialized
# $gp is not initialized
#
# C0_STATUS is same as YAMON™ context, but interrupts are disabled.

LEAF(_start)
        # Load global pointer to make data section addressing possible
        LA      gp, _gp

        # Mask out software and hardware interrupts,
        # ensure we're in kernel mode, disable FPU,
        # leave error level & exception level and disable interrupts.
        mfc0    t0, C0_STATUS
        li      t1, ~(SR_IPL_MASK|SR_KSU_MASK|SR_CU1|SR_ERL|SR_EXL|SR_IE)
        and     t0, t1
        mtc0    t0, C0_STATUS

        # Clear pending software and hardware interrupts
        mfc0    t0, C0_CAUSE
        li      t1, ~CR_IP_MASK
        and     t0, t1
        mtc0    t0, C0_CAUSE

        # Initialize the platform
        jal     platform_init
        nop

        # Switch to kernel main thread
        LOAD_PCPU(t0)
        move    a0, zero
        PTR_L   a1, PCPU_CURTHREAD(t0)
        j       ctx_switch
        nop
END(_start)

# vim: sw=8 ts=8 et
