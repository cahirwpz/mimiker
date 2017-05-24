#ifndef _SYS_PMAP_H_
#define _SYS_PMAP_H_

#include <vm.h>
#include <queue.h>

typedef uint8_t asid_t;
typedef uint32_t pte_t;
typedef uint32_t pde_t;

typedef struct pmap {
  pte_t *pte;          /* page table */
  pte_t *pde;          /* directory page table */
  vm_page_t *pde_page; /* pointer to a page with directory page table */
  TAILQ_HEAD(, vm_page) pte_pages; /* pages we allocate in page table */
  vm_addr_t start, end;
  asid_t asid;
} pmap_t;

pmap_t *pmap_new(void);
void pmap_reset(pmap_t *pmap);
void pmap_delete(pmap_t *pmap);

bool pmap_is_mapped(pmap_t *pmap, vm_addr_t vaddr);
bool pmap_is_range_mapped(pmap_t *pmap, vm_addr_t start, vm_addr_t end);

void pmap_map(pmap_t *pmap, vm_addr_t start, vm_addr_t end, pm_addr_t paddr,
              vm_prot_t prot);
void pmap_protect(pmap_t *pmap, vm_addr_t start, vm_addr_t end, vm_prot_t prot);
void pmap_unmap(pmap_t *pmap, vm_addr_t start, vm_addr_t end);
bool pmap_probe(pmap_t *pmap, vm_addr_t start, vm_addr_t end, vm_prot_t prot);

void pmap_activate(pmap_t *pmap);
pmap_t *get_kernel_pmap(void);
pmap_t *get_user_pmap(void);

void tlb_exception_handler(void);

#endif /* !_SYS_PMAP_H_ */
