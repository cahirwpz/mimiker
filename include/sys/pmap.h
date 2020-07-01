#ifndef _SYS_PMAP_H_
#define _SYS_PMAP_H_

#include <sys/vm.h>
#include <sys/queue.h>
#include <sys/exception.h>
#include <sys/mutex.h>
#include <machine/pmap.h>

/*
 * Flags passed to pmap_enter may contain VM_PROT_* read/write/execute
 * permissions. This information may be used to seed modified/referenced
 * information for the page being mapped, possibly avoiding redundant faults
 * on platforms that track modified/referenced information in software
 * like some MIPS and AArch64 processors.
 *
 * 00000ccc 00000000 00000000 00000ppp
 *
 * (c) cache bits
 * (p) protection bits
 */

#define PMAP_PROT_MASK VM_PROT_MASK
#define PMAP_CACHE_SHIFT 24

#define PMAP_NOCACHE (1 << PMAP_CACHE_SHIFT)
#define PMAP_WRITE_THROUGH (2 << PMAP_CACHE_SHIFT)
#define PMAP_WRITE_BACK (3 << PMAP_CACHE_SHIFT)
#define PMAP_CACHE_MASK (7 << PMAP_CACHE_SHIFT)

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

void init_pmap(void);

pmap_t *pmap_new(void);
void pmap_reset(pmap_t *pmap);
void pmap_delete(pmap_t *pmap);

void pmap_enter(pmap_t *pmap, vaddr_t start, vm_page_t *page, vm_prot_t prot,
                unsigned flags);
void pmap_protect(pmap_t *pmap, vaddr_t start, vaddr_t end, vm_prot_t prot);
void pmap_remove(pmap_t *pmap, vaddr_t start, vaddr_t end);
bool pmap_extract(pmap_t *pmap, vaddr_t va, paddr_t *pap);

void pmap_kenter(vaddr_t va, paddr_t pa, vm_prot_t prot, unsigned flags);
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
