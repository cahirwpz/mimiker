#ifndef _PMAP_H_
#define _PMAP_H_

#include <tlb.h>
#include <vm.h>
#include <queue.h>

/* Page is in every address space, note that this sets up only one page table */
#define PMAP_GLOBAL G_MASK
/* Access to page with PMAP_VALID won't cause exception */
#define PMAP_VALID  V_MASK
/* Writing to page with PMAP_DIRTY won't cause exception */
#define PMAP_DIRTY  D_MASK

#define PMAP_NONE 0

#define NULL_PHYS_PAGE 0

typedef uint32_t pte_t;

#define PTE_ENTRIES  (1024*1024)
#define PTE_SIZE     (PTE_ENTRIES*sizeof(pte_t))

typedef uint8_t asid_t;

typedef struct pmap {
  pte_t *pte;  /* page table */
  pte_t *pde; /* directory page table */
  vm_page_t *pde_page; /* pointer to page allocated from vm_phys containing dpt */
  TAILQ_HEAD(, vm_page) pte_pages; /* pages we allocate in page table */
  asid_t asid;
} pmap_t;

void pmap_init(pmap_t *pmap);
void pmap_map(pmap_t *pmap, vm_addr_t vaddr, pm_addr_t paddr, size_t npages,
              uint8_t flags);
void pmap_unmap(pmap_t *pmap, vm_addr_t vaddr, size_t npages);
void set_active_pmap(pmap_t *pmap);
pmap_t *get_active_pmap();

#endif /* !_PMAP_H_ */

