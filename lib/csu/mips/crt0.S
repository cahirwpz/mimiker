#include <mips/asm.h>
#include <mips/regdef.h>

	.set noreorder
	.global _start

# This is the starting procedude for user ELF images, used as their
# entry point. It takes care of preparing argc and argc for main().
# For details about environment in which this procedure starts, refer
# to prepare_program_stack in exec.c.

NESTED(_start, CALLFRAME_SIZ, ra)
        # Adhere to calling convention wrt return address so that GDB can
        # properly recognize stack frames.
        # However, store zero as RA, to mark stack bottom.
        addiu   sp, sp, -8
        li      ra, 0x0
        sw      ra, 4(sp)
        sw      sp, 0(sp)

        # Prepare $gp
        la      gp, _gp

        # Grab argc from below stack
        lw      a0, 8(sp)
        # Prepare argv pointing at argument vector below stack
        addiu   a1, sp, 12

        # Prepare envp, it starts at argv + argc + 1
        move    t0, a0
        addi    t0, 1
        sll     t0, 2
        add     a2, t0, a1
	
        # Jump to start in crt0-common.c
        j       ___start
        nop
END(_start)

# vim: sw=8 ts=8 et
