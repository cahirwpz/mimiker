#include <mips/asm.h>
#include <mips/pcpu.h>
#include <mips/regdef.h>
#include <mips/m32c0.h>

#include "assym.h"

        .set noreorder
        .section .boot, "ax"

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
        # Use stack pointer provided by linker script
        LA      sp, _sp

        # Load global pointer to make data section addressing possible
        LA      gp, _gp

        # Save registers and initialize the processor
        move    s0, a0
        move    s1, a1
        move    s2, a2
        move    s3, a3

        jal     mips_init
        nop

        # Restore kernel arguments and initialize the platform
        move    a0, s0
        move    a1, s1
        move    a2, s2
        move    a3, s3

        la      t0, platform_init
        jalr    t0
        nop

        # Switch to kernel main thread
        LOAD_PCPU(t0)
        PTR_L   a1, PCPU_CURTHREAD(t0)
        la      t0, ctx_switch
        jalr    t0
        move    a0, zero                # (delay) 1st arg
END(_start)

# vim: sw=8 ts=8 et
