#include <mips/ctx.h>
#include <mips/m32c0.h>
#include <mips/regdef.h>
#include <mips/pmap.h>
#include <vm.h>

#include "assym.h"

        # Don't allow the assembler to reorder instructions.
        .set	noreorder
        # Forbid the assembler from using $at register.
        .set	noat

        .global _ebase
        .type   tlb_refill,@function

        .section .ebase, "ax"

_ebase:

        .org    0x0
        .local  tlb_refill
        .local  pde_valid
        .local  pde_invalid

# TLB miss handler must avoid generating TLB miss during PT search.
# 
# If you use TLBR instruction remember that it overwrites EntryHi!
#
# Please verify EHB (clear execution hazard) instruction placement against
# chapter 2.8 of "MIPS32® 24KETM Processor Core Family Software User’s Manual".
# EHB has to be put between a producer and a consumer - preferably just before
# the consumer, not just after the producer.

tlb_refill:
        # Read PDE associated with bad virtual address.
        # Highest bit of the address switches between UPD_BASE & KPD_BASE,
        # so it's copied into 12th position with arithmetic shift.
        mfc0    k1, C0_BADVADDR
        sra     k1, 20
        andi    k1, 0x1ffc
        la      k0, UPD_BASE
        addu    k0, k1
        lw      k1, (k0)                # [k1] PDE of bad virtual address

        # Is PDE valid?
        andi    k0, k1, ENTRYLO0_V_MASK
        bnez    k0, pde_valid
        nop

pde_invalid:
        # PDE is invalid, so PTEs do not exists - simulate that by writing
        # invalid entries into the TLB. Retrying the instruction that missed
        # in the TLB is going to generate a TLB Invalid exception (but not
        # another TLB Refill exception).
        mtc0    zero, C0_ENTRYLO0
        mtc0    zero, C0_ENTRYLO1
        ehb
        tlbwr
        eret

pde_valid:
        # Calculate page table address from PDE
        srl     k1, 6
        sll     k1, 12
        # ... and index of even entry corresponding to bad virtual address.
        mfc0    k0, C0_BADVADDR
        srl     k0, 10
        andi    k0, 0xff8
        addu    k0, k1

        # Simply grab the PTEs and load them into the TLB.
        lw      k1, 0(k0)
        mtc0    k1, C0_ENTRYLO0
        lw      k1, 4(k0)
        mtc0    k1, C0_ENTRYLO1
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
