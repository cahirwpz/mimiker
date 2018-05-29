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
        .local  hit
        .local  miss
        .local  valid
        .local  invalid
        .local  pt_access

# TLB miss handler must avoid generating TLB miss during PT search.
# 
# Notes for the reader:
#  - tlbr instruction overwrites EntryHi, so we have restore it,
#  - handler does not fit into 256B, so it is splitted into two parts.
#
# Please verify EHB (clear execution hazard) instruction placement against
# chapter 2.8 of "MIPS32® 24KETM Processor Core Family Software User’s Manual".
# EHB has to be put between a producer and a consumer - preferably just before
# the consumer, not just after the producer.

tlb_refill:
        # Consider special case when an access to page table region was from
        # the kernel (namely pmap module). As UPD & KPD are not mapped within
        # page table we have to deal with it separately :(

        mfc0    k0, C0_BADVADDR
        srl     k1, k0, PT_SIZE_BITS
        addi    k1, k1, -(PT_BASE >> PT_SIZE_BITS)
        beqz    k1, pt_access
        nop

        mfc0    k0, C0_CONTEXT
        sra     k0, 1

        # Is it safe to access the PT? (is there a valid entry in the TLB?)
        mfc0    k1, C0_ENTRYHI  # Save original EntryHi
        ins     k0, k1, 0, C0_ENTRYHI_VPN2_SHIFT # Copy current ASID
        mtc0    k0, C0_ENTRYHI
        ehb
        tlbp
        ehb
        mtc0    k1, C0_ENTRYHI  # Restore original EntryHi
        mfc0    k0, C0_INDEX
        bltz    k0, miss        # Index < 0 => no entry with matching VPN2
        nop

maybe_hit:
        # We found an entry with a matching VPN2, but that doesn't mean that
        # it's valid! We need to check it.

        tlbr                    # Read the entry with matching VPN2
        ehb
        mtc0    k1, C0_ENTRYHI  # Restore original EntryHi

        # Have we accessed an even or odd page?
        mfc0    k0, C0_CONTEXT
        sra     k0, 1
        andi    k1, k0, PAGESIZE
        bnez    k1, 1f
        mfc0    k1, C0_ENTRYLO1         # Odd page => k1 = EntryLo1
        mfc0    k1, C0_ENTRYLO0         # Even page => k1 = EntryLo0
1:      andi    k1, ENTRYLO0_V_MASK     # Is the entry valid?
        beqz    k1, invalid
        nop

hit:
        # The entry is valid, so it's safe to access the PT (no TLB miss!)
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
        # Otherwise lookup in KPD. Preconditions: k0 == BadVAddr.

        lui     k1, %hi(PT_BASE)
        subu    k1, k0, k1              # k1 = offset into PT (22 bits)
        srl     k0, k1, 21              # k0 = if <accessing user part> then 0 else 1
        sll     k0, PTE_SHIFT           # k0 = if <accessing user part> then 0 else PAGESIZE
        srl     k1, PTE_SHIFT + 1       # k1 = index of even PDE in pair / 2
        sll     k1, 3                   # k1 = offset into PD
        addu    k0, k1, k0
        lui     k1, %hi(PD_BASE)
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
        # BadVAddr in kuseg => look in UPD; in kseg2 => look in KPD.

        mfc0    k1, C0_BADVADDR
        srl     k0, k1, PDE_SHIFT       # k0 = PDE index
        sll     k0, 2                   # k0 = PDE offset = PDE index * 4
        bgez    k1, 1f                  # BadVaddr in kuseg?
        lui     k1, %hi(PD_BASE)        # yes => k1 = UPD address
        addi    k1, PAGESIZE            # no => k1 = KPD address
1:      addu    k0, k1                  # k0 = PDE address
        lw      k1, 0(k0)               # Load PDE!
        andi    k1, ENTRYLO0_V_MASK     # Is it valid?
        beqz    k1, invalid
        # Note: no nop inside delay slot -- executing the ins instruction
        # won't hurt us if we jump to invalid.

valid:
        # Load two consecutive PDEs into EntryLo0 and EntryLo1
        ins     k0, zero, 2, 1          # Align PDE address to even index.
        lw      k1, 0(k0)
        mtc0    k1, C0_ENTRYLO0
        lw      k1, 4(k0)
        mtc0    k1, C0_ENTRYLO1

        # Determine EntryHi value base on PDE address. We need to choose right
        # ASID for it: BadVaddr in kuseg => ASID of user thread; otherwise 0.
        mfc0    k1, C0_CONTEXT
        sra     k1, 1
        ins     k1, zero, 0, C0_ENTRYHI_VPN2_SHIFT # EntryHi with ASID=0
        mfc0    k0, C0_BADVADDR
        bltz    k0, 1f                  # BadVaddr in kseg?
        mfc0    k0, C0_ENTRYHI          # Save original EntryHi
        ins     k1, k0, 0, C0_ENTRYHI_VPN2_SHIFT # Fill in with thread's ASID
1:      mtc0    k1, C0_ENTRYHI
        ehb
        tlbwr
        ehb
        mtc0    k0, C0_ENTRYHI          # Restore original EntryHi
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
