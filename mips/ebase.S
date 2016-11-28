#include <mips/ctx.h>
#include <mips/m32c0.h>

#include "assym.h"

        # Don't allow the assembler to reorder instructions.
        .set	noreorder
        # Forbid the assembler from using $at register.
        .set	noat

        .global _ebase
        .type   tlb_refill,@function

        .section .ebase

_ebase:

        .org 0x0

tlb_refill:
        mfc0    $k0, C0_CONTEXT         # Load PT address which contains PTEs
        mfc0    $k1, C0_BADVADDR
        andi    $k1, 0x1000
        bnez    $k1, 2f
        sra     $k0, 1                  # PT is at the beginning of KSEG2

        # Both cases can fail if corresponding PDE is not in TLB.
        # We want to fail first on address that refer to the faulted page,
        # so we can recover correct address in tlb_exception_handler.
1:      lw      $k1, 0($k0)
        mtc0    $k1, C0_ENTRYLO0
        lw      $k1, 4($k0)
        mtc0    $k1, C0_ENTRYLO1
        ehb                             # clear hazard on tlbwr
        tlbwr
        eret

2:      lw      $k1, 4($k0)
        mtc0    $k1, C0_ENTRYLO1
        lw      $k1, 0($k0)
        mtc0    $k1, C0_ENTRYLO0
        ehb                             # clear hazard on tlbwr
        tlbwr
        eret

        .org 0x100

cache_error:
1:      j       1b
        nop

        .org 0x180

general_exception:
        mfc0    $k1, C0_CAUSE
        andi    $k1, CR_X_MASK
        la      $k0, general_exception_table
        add     $k1, $k0
        lw      $k1, 0($k1)             # Load the address of a handler
        bnez    $k1, exc_enter          # If non-NULL then enter handler
        mfc0    $k0, C0_STATUS          # (delay slot) load status register
        la      $k1, kernel_oops        # If no handler then panic!
        j       exc_enter
        nop

        .org 0x200
irq0:
1:      j       1b
        nop

        .org 0x220
irq1:
1:      j       1b
        nop

        .org 0x240
irq2:
1:      j       1b
        nop

        .org 0x260
irq3:
1:      j       1b
        nop

        .org 0x280
irq4:
1:      j       1b
        nop

        .org 0x2a0
irq5:
1:      j       1b
        nop

        .org 0x2c0
irq6:
1:      j       1b
        nop

        .org 0x2e0
irq7:
        la      $k1,mips_clock_irq_handler
        j       exc_enter
        mfc0    $k0, C0_STATUS          # (delay slot) load status register

# vim: sw=8 ts=8 et
