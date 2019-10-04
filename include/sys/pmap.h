#ifndef _SYS_PMAP_H_
#define _SYS_PMAP_H_

#include <sys/vm.h>
#include <sys/queue.h>
#include <sys/exception.h>
#include <sys/mutex.h>

typedef uint8_t asid_t;
typedef uint32_t pte_t;
typedef uint32_t pde_t;

typedef struct pmap {
  pte_t *pde;          /* directory page table */
  vm_page_t *pde_page; /* pointer to a page with directory page table */
  pg_list_t pte_pages; /* pages we allocate in page table */
  vaddr_t start, end;
  asid_t asid;
  mtx_t mtx;
} pmap_t;

void pmap_init(void);

pmap_t *pmap_new(void);
void pmap_reset(pmap_t *pmap);
void pmap_delete(pmap_t *pmap);

void pmap_enter(pmap_t *pmap, vaddr_t start, vm_page_t *page, vm_prot_t prot);
void pmap_protect(pmap_t *pmap, vaddr_t start, vaddr_t end, vm_prot_t prot);
void pmap_remove(pmap_t *pmap, vaddr_t start, vaddr_t end);

void pmap_zero_page(vm_page_t *pg);
void pmap_copy_page(vm_page_t *src, vm_page_t *dst);

bool pmap_clear_modified(vm_page_t *pg);
bool pmap_clear_referenced(vm_page_t *pg);
bool pmap_is_modified(vm_page_t *pg);
bool pmap_is_referenced(vm_page_t *pg);
void pmap_set_referenced(paddr_t pa);
void pmap_set_modified(paddr_t pa);

void pmap_activate(pmap_t *pmap);
pmap_t *get_kernel_pmap(void);
pmap_t *get_user_pmap(void);

void tlb_exception_handler(exc_frame_t *frame);

#endif /* !_SYS_PMAP_H_ */
