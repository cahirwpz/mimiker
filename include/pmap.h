#ifndef _PMAP_H_
#define _PMAP_H_

#include <vm.h>
#include <queue.h>

#define PTE_EXT_MODIFIED    0x00000040
#define PTE_EXT_REFERENCED  0x00000020

typedef uint8_t asid_t;

typedef struct pde {
  uint32_t lo;
  uint32_t unused;
} pde_t;

typedef struct pte {
  uint32_t lo;
  uint32_t ext_flags;
} pte_t;

typedef struct pmap {
  pte_t *pte;          /* page table */
  pde_t *pde;          /* directory page table */
  vm_page_t *pde_page; /* pointer to the first page with directory page table */
  TAILQ_HEAD(, vm_page) pte_pages; /* pages we allocate in page table */
  vm_addr_t start, end;
  asid_t asid;
} pmap_t;

void pmap_init();
pmap_t *pmap_new();
void pmap_reset(pmap_t *pmap);
void pmap_delete(pmap_t *pmap);

void pmap_map(pmap_t *pmap, vm_addr_t start, vm_addr_t end, pm_addr_t paddr,
              vm_prot_t prot);
void pmap_protect(pmap_t *pmap, vm_addr_t start, vm_addr_t end, vm_prot_t prot);
void pmap_unmap(pmap_t *pmap, vm_addr_t start, vm_addr_t end);
bool pmap_probe(pmap_t *pmap, vm_addr_t start, vm_addr_t end, vm_prot_t prot);

void pmap_activate(pmap_t *pmap);
pmap_t *get_kernel_pmap();
pmap_t *get_user_pmap();

typedef struct exc_frame exc_frame_t;

void tlb_exception_handler(exc_frame_t *frame);
void tlb_load_exception_handler(exc_frame_t *frame);
void tlb_store_exception_handler(exc_frame_t *frame);
void tlb_modified_exception_handler(exc_frame_t *frame);


#endif /* _PMAP_H_ */
