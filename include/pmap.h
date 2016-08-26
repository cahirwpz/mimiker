#ifndef _PMAP_H_
#define _PMAP_H_

#include <vm.h>
#include <queue.h>

typedef enum {PMAP_KERNEL, PMAP_USER, PMAP_LAST} pmap_type_t;

typedef uint8_t asid_t;
typedef uint32_t pte_t;
typedef uint32_t pde_t;

typedef struct {
  pmap_type_t type;
  pte_t *pte; /* page table */
  pte_t *pde; /* directory page table */
  vm_page_t *pde_page; /* pointer to a page with directory page table */
  TAILQ_HEAD(, vm_page) pte_pages; /* pages we allocate in page table */
  vm_addr_t start, end;
  asid_t asid;
} pmap_t;

void pmap_setup(pmap_t *pmap, pmap_type_t type, asid_t asid);
void pmap_reset(pmap_t *);

void pmap_map(pmap_t *pmap, vm_addr_t start, vm_addr_t end, pm_addr_t paddr,
              vm_prot_t prot);
void pmap_protect(pmap_t *pmap, vm_addr_t start, vm_addr_t end,
                  vm_prot_t prot);
void pmap_unmap(pmap_t *pmap, vm_addr_t start, vm_addr_t end);
bool pmap_probe(pmap_t *pmap, vm_addr_t start, vm_addr_t end, vm_prot_t prot);

void set_active_pmap(pmap_t *pmap);
pmap_t *get_active_pmap(pmap_type_t type);

__attribute__((interrupt)) void tlb_exception_handler();

#endif /* _PMAP_H_ */
