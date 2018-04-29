#include <mips/ctx.h>
#include <mips/m32c0.h>
#include <mips/regdef.h>
#include <mips/pmap.h>
#include <vm.h>

#include "assym.h"

#define PT_BASE_UPPER (PT_BASE >> 16)
#define PD_BASE_UPPER (PD_BASE >> 16)

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
        .local  hit
        .local  miss
        .local  valid
        .local  invalid
        .local  pt_access

tlb_refill:
        # We don't want to generate TLB misses during tlb_refill.

        # Special case: an access was made to the PT from outside tlb_refill.
        # This needs to be checked and handled separately.

        mfc0    k0, C0_BADVADDR
        srl     k1, k0, 22
        addi    k1, k1, -(PT_BASE >> 22)
        beqz    k1, pt_access
        nop

        mfc0    k0, C0_CONTEXT
        sra     k0, 1                                   # k0 = address of PTE of even page from the pair

        # Check if it's safe to access the PT (is there a valid entry in the TLB?)
        mfc0    k1, C0_ENTRYHI                          # Save original EntryHi for later.
        ins     k0, k1, 0, C0_ENTRYHI_VPN2_SHIFT        # Copy current ASID.
        mtc0    k0, C0_ENTRYHI
        ehb
        tlbp
        ehb
        mtc0    k1, C0_ENTRYHI                          # Restore original EntryHi.
        mfc0    k0, C0_INDEX
        # Index < 0 means there was no entry with matching VPN2.
        bltz    k0, miss
        nop
maybe_hit:
        # We found an entry with a matching VPN2, but that doesn't mean that it's valid!
        # We need to check it.

        # Read the entry with matching VPN2 (note: tlbr overwrites EntryHi!)
        tlbr
        mtc0    k1, C0_ENTRYHI                  # Restore original EntryHi.

        mfc0    k0, C0_CONTEXT
        sra     k0, 1
        ext     k1, k0, PAGE_SHIFT, 1           # Check if we accessed an even or odd page.
        ehb

        # If we accessed an even page, k1 == 0 and both mfc0 instructions will be executed,
        # so the final value of k1 will be EntryLo0.
        # If we accessed an odd page, k1 == 1 and only the first mfc0 instruction will be
        # executed, so the final value of k1 will be EntryLo1.
        bnez    k1, 1f
        mfc0    k1, C0_ENTRYLO1
        mfc0    k1, C0_ENTRYLO0
1:
        andi    k1, ENTRYLO0_V_MASK             # Is the entry valid?
        beqz    k1, invalid
        nop
hit:
        # The entry is valid, so it's safe to access the PT (we won't miss in the TLB).
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

pt_access:
        # If we're accessing the user part of the PT, take PDE from UPD.
        # Otherwise lookup in KPD.

        # Precondition:
        # k0 == BadVAddr

        lui     k1, PT_BASE_UPPER
        subu    k1, k0, k1              # k1 = offset into PT (22 bits)
        srl     k0, k1, 21              # k0 = if <accessing user part> then 0 else 1
        sll     k0, PAGE_SHIFT          # k0 = if <accessing user part> then 0 else PAGESIZE
        srl     k1, PAGE_SHIFT + 1      # k1 = index of even PDE in pair / 2
        sll     k1, 3                   # k1 = offset into PD
        addu    k0, k1, k0
        lui     k1, PD_BASE_UPPER
        addu    k0, k1, k0
        lw      k1, 0(k0)               # Load even PDE.
        mtc0    k1, C0_ENTRYLO0
        lw      k1, 4(k0)               # Load odd PDE.
        mtc0    k1, C0_ENTRYLO1
        ehb
        tlbwr
        eret
miss:
        # Find PDE mapping the Page Table Fragment we want to access.
        # If BadVAddr is in kuseg, look in user PD. If it's in kseg2, look in kernel PD.
        mfc0    k1, C0_BADVADDR
        srl     k0, k1, PDE_SHIFT               # k0 = PDE index
        sll     k0, 2                           # k0 = PDE offset = PDE index * 4
        srl     k1, 31
        sll     k1, PAGE_SHIFT                  # k1 = <BadVAddr in kuseg> ? 0 : PAGESIZE
        addu    k0, k0, k1
        lui     k1, PD_BASE_UPPER
        addu    k0, k0, k1                      # k0 = address of PDE
        lw      k1, 0(k0)                       # Load PDE.
        andi    k1, ENTRYLO0_V_MASK             # Is it valid?
        beqz    k1, invalid
        # Note: no nop inside delay slot -- executing the ins instruction
        # won't hurt us if we jump to invalid.

valid:
        # Load the PDE into the TLB.
        ins     k0, zero, 2, 1                  # Clear bit 2 (3rd least significant bit).
        lw      k1, 0(k0)
        mtc0    k1, C0_ENTRYLO0
        lw      k1, 4(k0)
        mtc0    k1, C0_ENTRYLO1

        # Check if we accessed user or kernel PD.
        li      k1, PD_BASE + PAGESIZE
        sub     k1, k0, k1                      # k1 < 0 if we accessed user PD.
        sra     k1, 31                          # k1 = <accessed user PD> ? 0xffffffff : 0
        mfc0    k0, C0_ENTRYHI
        and     k0, k0, k1
        mfc0    k1, C0_CONTEXT
        sra     k1, 1

        # If we accessed the user PD, the ASID of the TLB entry should be
        # the ASID of the thread that caused the exception.
        # Otherwise, the ASID should be 0 (ASID reserved for the kernel).
        ins     k1, k0, 0, C0_ENTRYHI_VPN2_SHIFT

        mfc0    k0, C0_ENTRYHI                  # Save original EntryHi.
        mtc0    k1, C0_ENTRYHI
        ehb
        tlbwr
        ehb
        mtc0    k0, C0_ENTRYHI                  # Restore original EntryHi.
        mfc0    k0, C0_CONTEXT
        j       hit
        sra     k0, 1
invalid:
        # The access is invalid, so write invalid entries into the TLB.
        # Retrying the instruction that missed in the TLB is going to generate
        # a TLB Invalid exception (but not another TLB Refill exception).
        mtc0    zero, C0_ENTRYLO0
        mtc0    zero, C0_ENTRYLO1
        ehb
        tlbwr
        eret
# vim: sw=8 ts=8 et
