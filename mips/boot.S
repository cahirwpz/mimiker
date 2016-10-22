#include <mips/asm.h>
#include <mips/m32c0.h>

        .set noreorder

        .local clear_bss
        .local kernel_go

# Boot environment is described in MIPS® YAMON™ User's Manual, Chapter 5
#
# $a0 = argc
# $a1 = argv
# $a2 = envp
# $a3 = memsize
#
# $sp is initialized
# $gp is not initialized
# C0_STATUS(SR_IE) is cleared

LEAF(_start)
        la      $t0, _memsize
        sw      $a3,($t0)
        la      $gp, _gp

clear_bss:
        la      $t0, __bss
        la      $t1, __ebss

1:      sw      $zero, ($t0)
        bne     $t0, $t1, 1b
        addiu   $t0, 4

kernel_go:
	/*
	* We will preserve SR_BEV settings for now, until we override it later on.
	* SR_SR (Soft Reset bit) will be preserved as well to support reboot in the future.
        */
        li      $t0, (SR_BEV | SR_SR)
        mfc0    $t1, C0_STATUS
        and     $t1, $t0
        mtc0    $t1, C0_STATUS
        ehb

	/* Transfer control to `kernel_boot`, and hang if it returns */
        la      $ra, kernel_exit
        la      $t0, kernel_boot
        jr      $t0
        nop
END(_start)

LEAF(kernel_exit)
1:      j       1b
        nop
END(kernel_exit)


        .globl _memsize
        .type  _memsize,@object
        .size  _memsize,4

        .data
_memsize:
        .long 0
