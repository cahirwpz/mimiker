#include <mips/asm.h>
#include <mips/regdef.h>

        .set    noreorder
        .globl  _C_LABEL(__errno)

NESTED(__sc_error, CALLFRAME_SIZ, ra)
        PTR_SUBU sp, CALLFRAME_SIZ
        .mask	0x80000000, (CALLFRAME_RA - CALLFRAME_SIZ)
        sw      ra, CALLFRAME_RA(sp)
        sw      v1, CALLFRAME_S0(sp)
        jal     __errno
        nop
        lw      v1, CALLFRAME_S0(sp)
        sw      v1, 0(v0)
        li      v0, -1
        lw      ra, CALLFRAME_RA(sp)
        jr      ra
        PTR_ADDU sp, CALLFRAME_SIZ
END(__sc_error)

# vim: sw=8 ts=8 et
