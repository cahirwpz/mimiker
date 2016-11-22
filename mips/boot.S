#include <mips/asm.h>
#include <mips/m32c0.h>

        .set noreorder

        .local kernel_go

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
        la      $gp, _gp
        jal     platform_init
        nop

kernel_go:
	# Let's preserve SR_BEV for now, until interrupts are propely set up
	# later. SR_SR (Soft Reset) bit will be preserved as well to support
	# reboot in the future.
        li      $t0, (SR_BEV | SR_SR)
        mfc0    $t1, C0_STATUS
        and     $t1, $t0
        mtc0    $t1, C0_STATUS
        ehb

	# Transfer control to `kernel_boot` running in kernel mode with
	# interrupts and FPU disabled.
	# If kernel ever decides to return just loop forever.
        la      $t0, _kenv
        lw      $a0, 0($t0)
        lw      $a1, 4($t0)
        la      $ra, kernel_exit
        la      $t0, kernel_boot
        jr      $t0
        nop
END(_start)

LEAF(kernel_exit)
1:      j       1b
        nop
END(kernel_exit)

# vim: sw=8 ts=8 et
