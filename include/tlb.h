#ifndef _TLB_H_
#define _TLB_H_

#include <common.h>
#include <mips.h>

#define PFN_MASK  0x03ffffc0
#define C_MASK    0x38
#define D_MASK    0x4
#define V_MASK    0x2
#define G_MASK    0x1
#define VPN_MASK  0xffffe000
#define ASID_MASK 0x000000ff

#define ENTRYLO_SET_PADDR(e,p)  (e) = ((e) & ~(PFN_MASK)) | (((p) & 0xfffff000) >> 6)
#define ENTRYLO_SET_C(e,c)      (e) = ((e) & ~(C_MASK))   | (((c) << 3) & C_MASK)
#define ENTRYLO_SET_D(e,d)      (e) = ((e) & ~(D_MASK))   | (((d) << 2) & D_MASK)
#define ENTRYLO_SET_V(e,b)      (e) = ((e) & ~(V_MASK))   | (((b) << 1) & V_MASK)
#define ENTRYLO_SET_G(e,g)      (e) = ((e) & ~(G_MASK))   | (((g)     ) & G_MASK)

#define ENTRYHI_SET_VADDR(e,v)  (e) = ((e) & ~(VPN_MASK))  | (v)
#define ENTRYHI_SET_ASID(e,a)   (e) = ((e) & ~(ASID_MASK)) | (a)

#define PTE_BASE  MIPS_KSEG2_START

void     tlb_init();
void     tlb_print();

/* Note that MIPS implements variuos page sizes by specifying PageMask register, so
 * intuitively these functions shall specify PageMask. However in current implementation
 * we aren't going to use other page size than 4KB. */


/* Returns the number of entries in the TLB.  */
unsigned tlb_size();

/* Probes the TLB for an entry matching hi, and if present invalidates it. */
void     tlb_invalidate(tlbhi_t hi);

/* Invalidate the entire TLB. */
void     tlb_invalidate_all();

/* Reads the TLB entry with specified by index, and returns the EntryHi, EntryLo0, EntryLo1, and
 * parts in *hi, *lo0, *lo1 respectively. */
void     tlb_read_index(tlbhi_t *hi, tlblo_t *lo0, tlblo_t *lo1, int index);


/* Writes hi, lo0, lo1 into the TLB entry specified by index. */
void     tlb_write_index(tlbhi_t hi, tlblo_t lo0, tlblo_t lo1, int i);

/* Writes hi, lo0, lo1 and msk into the TLB entry specified by the Random Register. */
void     tlb_write_random(tlbhi_t hi, tlblo_t lo0, tlblo_t lo1);

/* Probes the TLB for an entry matching hi and returns its index, or -1 if not found. If found, then the
 * EntryLo0, EntryLo1 and PageMask parts of the entry are also returned in *plo0, *plo1
 * respectively. */
void     tlb_probe2(tlbhi_t hi, tlblo_t *lo0, tlblo_t *lo1);

/* If there is entry matching hi overwrites it, else writes into random register.
 * Safest way to update TLB. */
void     tlb_overwrite_random(tlbhi_t hi, tlblo_t lo0, tlblo_t lo1);

#endif /* _TLB_H */

