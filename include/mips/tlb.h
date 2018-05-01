#ifndef _MIPS_TLB_H_
#define _MIPS_TLB_H_

#include <stdint.h>

typedef uint32_t tlbhi_t;
typedef uint32_t tlblo_t;

/* Choose index specified by the Random Register. */
#define TLBI_RANDOM (-1U)

typedef struct {
  tlbhi_t hi;
  tlblo_t lo0;
  tlblo_t lo1;
} tlbentry_t;

#define MAX_ASID C0_ENTRYHI_ASID_MASK

/* C0_CONTEXT register */
#define PTEBASE_MASK 0xff800000
#define BADVPN2_MASK 0x007ffff0
#define BADVPN2_SHIFT 9

/* MIPS® Architecture For Programmers Volume III, section 9.6 */
#define PTE_NO_READ 0x80000000
#define PTE_NO_EXEC 0x40000000
#define PTE_PFN_MASK 0x03ffffc0
#define PTE_PFN_SHIFT 6
#define PTE_CACHE_MASK 0x00000038
#define PTE_CACHE_SHIFT 3
#define PTE_DIRTY 0x00000004
#define PTE_VALID 0x00000002
#define PTE_GLOBAL 0x00000001
#define PTE_PROT_MASK (PTE_NO_READ | PTE_NO_EXEC | PTE_DIRTY | PTE_VALID)

#define PTE_PFN(addr) (((addr) >> PTE_PFN_SHIFT) & PTE_PFN_MASK)
#define PTE_CACHE(cache) (((cache) << PTE_CACHE_SHIFT) & PTE_CACHE_MASK)
#define PTE_PFN_OF(pte) (((pte)&PTE_PFN_MASK) << PTE_PFN_SHIFT)
#define PTE_CACHE_OF(pte) (((cache)&PTE_CACHE_MASK) >> PTE_CACHE_MASK)

#define PTE_LO_INDEX_MASK 0x00001000
#define PTE_LO_INDEX_SHIFT 12
#define PTE_LO_INDEX_OF(vaddr)                                                 \
  (((vaddr)&PTE_LO_INDEX_MASK) >> PTE_LO_INDEX_SHIFT)

#define PTE_VPN2_MASK 0xffffe000
#define PTE_ASID_MASK 0x000000ff

#define PTE_VPN2(addr) ((addr)&PTE_VPN2_MASK)
#define PTE_ASID(asid) ((asid)&PTE_ASID_MASK)

void tlb_init(void);
void tlb_print(void);

/*
 * Note that MIPS implements variable page size by specifying PageMask register,
 * so intuitively these functions shall specify PageMask. However in current
 * implementation we aren't going to use other page size than 4KiB.
 */

/* Returns the number of entries in the TLB. */
unsigned tlb_size(void);

/* Probes the TLB for an entry matching hi, and if present invalidates it. */
void tlb_invalidate(tlbhi_t hi);

/* Invalidate all TLB entries (save wired). */
void tlb_invalidate_all(void);

/* Invalidate all TLB entries with given ASID (save wired). */
void tlb_invalidate_asid(tlbhi_t hi);

/* Reads the TLB entry specified by @i. */
void tlb_read(unsigned i, tlbentry_t *e);

/* Writes the TLB entry specified by @i. */
void tlb_write(unsigned i, tlbentry_t *e);

/* Probes the TLB for an entry matching hi and if present rewrites that
 * entry, otherwise updates a random entry. A safe way to update the TLB. */
void tlb_overwrite_random(tlbentry_t *e);

/* Probes the TLB for an entry matching @e->hi. If found then @e will be filled
 * with EntryHi, EntryLo0, EntryLo1 values and index will be returned.
 * Otherwise tlb_probe will return with negative value. */
int tlb_probe(tlbentry_t *e);

#endif /* !_MIPS_TLB_H_ */
