#include <mips/ctx.h>
#include <mips/m32c0.h>
#include <mips/regdef.h>

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
        mfc0    k0, C0_CONTEXT          # Load PT address which contains PTEs
        mfc0    k1, C0_BADVADDR
        andi    k1, 0x1000
        bnez    k1, 2f
        sra     k0, 1                   # PT is at the beginning of KSEG2

        # Both cases can fail if corresponding PDE is not in TLB.
        # We want to fail first on address that refer to the faulted page,
        # so we can recover correct address in tlb_exception_handler.
1:      lw      k1, 0(k0)
        mtc0    k1, C0_ENTRYLO0
        lw      k1, 4(k0)
        mtc0    k1, C0_ENTRYLO1
        ehb                             # clear hazard on tlbwr
        tlbwr
        eret

2:      lw      k1, 4(k0)
        mtc0    k1, C0_ENTRYLO1
        lw      k1, 0(k0)
        mtc0    k1, C0_ENTRYLO0
        ehb                             # clear hazard on tlbwr
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
        la      k1, mips_exc_handler
        j       exc_enter
        mfc0    k0, C0_STATUS           # (delay slot) load status register

        .org    0x200
        .local  irq

irq:
        la      k1, mips_intr_handler
        j       exc_enter
        mfc0    k0, C0_STATUS           # (delay slot) load status register

# vim: sw=8 ts=8 et
