#ifndef _SYS_PMAP_H_
#define _SYS_PMAP_H_

#include <sys/vm.h>
#include <sys/queue.h>
#include <sys/exception.h>
#include <sys/mutex.h>
#include <machine/pmap.h>

#define PMAP_NOCACHE 0x00000001
#define PMAP_WRITE_THROUGH 0x00000002
#define PMAP_WRITE_BACK 0x00000004

typedef struct pmap {
  pde_t *pde;              /* directory page table */
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

void pmap_enter(pmap_t *pmap, vaddr_t start, vm_page_t *page, vm_prot_t prot,
                int flags);
void pmap_protect(pmap_t *pmap, vaddr_t start, vaddr_t end, vm_prot_t prot);
void pmap_remove(pmap_t *pmap, vaddr_t start, vaddr_t end);
bool pmap_extract(pmap_t *pmap, vaddr_t va, paddr_t *pap);

void pmap_kenter(vaddr_t va, paddr_t pa, vm_prot_t prot, int flags);
void pmap_kremove(vaddr_t start, vaddr_t end);

void pmap_zero_page(vm_page_t *pg);
void pmap_copy_page(vm_page_t *src, vm_page_t *dst);

bool pmap_clear_modified(vm_page_t *pg);
bool pmap_clear_referenced(vm_page_t *pg);
bool pmap_is_modified(vm_page_t *pg);
bool pmap_is_referenced(vm_page_t *pg);
void pmap_set_referenced(vm_page_t *pg);
void pmap_set_modified(vm_page_t *pg);

void pmap_activate(pmap_t *pmap);

pmap_t *pmap_lookup(vaddr_t va);
pmap_t *pmap_kernel(void);
pmap_t *pmap_user(void);

void tlb_exception_handler(exc_frame_t *frame);

#endif /* !_SYS_PMAP_H_ */
