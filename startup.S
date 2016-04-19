#include <asm.h>
#include <mips/m32c0.h>

        .set noreorder

        .local clear_bss
        .local kernel_go
        .local kernel_exit

# Boot environment is described in MIPS® YAMON™ User's Manual, Chapter 5
#
# $a0 = argc
# $a1 = argv
# $a2 = envp
# $a3 = memsize
#
# $sp is initialized

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
        # Status.ERL is set by the processor when a Reset, Soft Reset, NMI or
        # Cache Error exception are taken. When ERL is set:
        # - The processor is running in kernel mode
        # - Hardware and software interrupts are disabled
        # - The ERET instruction will use the return address held in ErrorEPC
        #   instead of EPC

        la      $ra, kernel_exit
        la      $t0, kernel_main
        mtc0    $t0, C0_ERRPC    # Return from exception to kernel_main
        eret                     # exception => normal level (SR_ERL = 0)

kernel_exit:
        # If for some reason kernel_main returned call a debugger.
        sdbbp
        nop

END(_start)

        .globl _memsize
        .type  _memsize,@object
        .size  _memsize,4

        .data
_memsize:
        .long 0
