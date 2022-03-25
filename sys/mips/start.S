#include <mips/asm.h>
#include <mips/pcpu.h>
#include <mips/regdef.h>

#include "assym.h"

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

# We're running on stack provided by the boot loader.
LEAF(_start)
        # Load global pointer to make data section addressing possible
        LA      gp, _gp

        # Save registers and initialize the processor
        move    s0, a0
        move    s1, a1
        move    s2, a2
        move    s3, a3

        jal     mips_init
        move    sp, v0                  # Set boot stack (already in kseg2)

        # From this point kernel sections are accessible through kseg2!
        move    a0, s0
        move    a1, s1
        move    a2, s2
        move    a3, s3

        LA      t0, board_stack
        jalr    t0
        move    sp, v0                  # Set thread0 stack 

        LA      t0, board_init
        jr      t0
END(_start)

# vim: sw=8 ts=8 et
