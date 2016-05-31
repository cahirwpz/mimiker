#ifndef _TLB_H_
#define _TLB_H_

#include <common.h> 
#include <mips.h>

typedef struct pmap pmap_t;
typedef uint32_t pte_t;

#define PT_BASE  MIPS_KSEG2_START
#define DPT_BASE (MIPS_KSEG2_START+0x300000)

#define PT_ENTRIES  (1024*1024)
#define DPT_ENTRIES 1024
#define PT_SIZE     (PT_ENTRIES*sizeof(pte_t))

#define PFN_MASK (0xfffff000 >> 6)
#define C_MASK   0x38
#define D_MASK   0x4
#define V_MASK   0x2
#define G_MASK   0x1

#define SET_PADDR(e,p)  (e) = ((e) & ~(PFN_MASK)) | (((p) & 0xfffff000) >> 6)
#define SET_C(e,c)      (e) = ((e) & ~(C_MASK)) | (((c) << 3) & C_MASK)
#define SET_D(e,d)      (e) = ((e) & ~(D_MASK)) | (((d) << 2) & D_MASK)
#define SET_V(e,b)      (e) = ((e) & ~(V_MASK)) | (((b) << 1) & V_MASK)
#define SET_G(e,g)      (e) = ((e) & ~(G_MASK)) | (((g)     ) & G_MASK)

void tlb_init();
void tlb_set_page_table(pmap_t *pmap);
void tlb_print();

#endif /* _TLB_H */

