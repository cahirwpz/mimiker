#ifndef _SYS_PMAP_H_
#define _SYS_PMAP_H_

#include <sys/vm.h>
#include <sys/queue.h>
#include <sys/exception.h>
#include <sys/mutex.h>
#include <machine/pmap.h>

typedef struct pmap {
  pte_t *pde;              /* directory page table */
  vm_page_t *pde_page;     /* pointer to a page with directory page table */
  vm_pagelist_t pte_pages; /* pages we allocate in page table */
  asid_t asid;
  mtx_t mtx;
} pmap_t;

bool pmap_address_p(pmap_t *pmap, vaddr_t va);
bool pmap_contains_p(pmap_t *pmap, vaddr_t start, vaddr_t end);
vaddr_t pmap_start(pmap_t *pmap);
vaddr_t pmap_end(pmap_t *pmap);

void pmap_bootstrap(void);

pmap_t *pmap_new(void);
void pmap_reset(pmap_t *pmap);
void pmap_delete(pmap_t *pmap);

void pmap_enter(pmap_t *pmap, vaddr_t start, vm_page_t *page, vm_prot_t prot);
void pmap_protect(pmap_t *pmap, vaddr_t start, vaddr_t end, vm_prot_t prot);
void pmap_remove(pmap_t *pmap, vaddr_t start, vaddr_t end);

void pmap_kenter(paddr_t va, paddr_t pa, vm_prot_t prot);

void pmap_zero_page(vm_page_t *pg);
void pmap_copy_page(vm_page_t *src, vm_page_t *dst);

bool pmap_clear_modified(vm_page_t *pg);
bool pmap_clear_referenced(vm_page_t *pg);
bool pmap_is_modified(vm_page_t *pg);
bool pmap_is_referenced(vm_page_t *pg);
void pmap_set_referenced(paddr_t pa);
void pmap_set_modified(paddr_t pa);

void pmap_activate(pmap_t *pmap);

pmap_t *pmap_lookup(vaddr_t va);
pmap_t *pmap_kernel(void);
pmap_t *pmap_user(void);

void tlb_exception_handler(exc_frame_t *frame);

#endif /* !_SYS_PMAP_H_ */
