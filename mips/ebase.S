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

        .org    0x0
        .local  tlb_refill

tlb_refill:
        mfc0    $k0, C0_CONTEXT
        lw      $k1, 0($k0)
        mtc0    $k1, C0_ENTRYLO0
        lw      $k1, 8($k0)
        mtc0    $k1, C0_ENTRYLO1
        ehb
        tlbwr
        eret

        .org    0x100
        .local  cache_error

cache_error:
1:      j       1b
        nop

        .org    0x180
        .local  general_exception

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

        .org    0x200
        .local  irq

irq:
        la      $k1,mips_irq_handler
        j       exc_enter
        mfc0    $k0, C0_STATUS          # (delay slot) load status register

# vim: sw=8 ts=8 et
