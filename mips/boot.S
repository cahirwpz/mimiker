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
        LA      gp, _gp
        jal     platform_init
        nop

        LOAD_PCPU(t0)
        move    a0, zero
        PTR_L   a1, PCPU_CURTHREAD(t0)
        j       ctx_switch
        nop
END(_start)

# vim: sw=8 ts=8 et
