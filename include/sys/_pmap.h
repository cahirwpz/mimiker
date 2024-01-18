#ifndef _SYS__PMAP_H_
#define _SYS__PMAP_H_

#ifndef _MACHDEP
#error "Do not use this header file outside kernel machine dependent code!"
#endif

#include <sys/mimiker.h>
#include <sys/mutex.h>
#include <sys/vm.h>
#include <machine/pmap.h>

struct pmap {
  mtx_t mtx;                      /* protects all fields in this structure */
  asid_t asid;                    /* address space identifier */
  paddr_t pde;                    /* directory page table physical address */
  vm_pagelist_t pte_pages;        /* pages we allocate in page table */
  TAILQ_HEAD(, pv_entry) pv_list; /* all pages mapped by this physical map */

  /* Machine-dependent part */
  pmap_md_t md;
};

typedef struct pv_entry {
  TAILQ_ENTRY(pv_entry) pmap_link; /* link on `pmap::pv_list` */
  TAILQ_ENTRY(pv_entry) page_link; /* link on `vm_page::pv_list` */
  pmap_t *pmap;                    /* page is mapped in this pmap */
  vaddr_t va;                      /* under this address */
} pv_entry_t;

/*
 * Each target must implement the following interface
 * along with the `pmap_md_t` struct and a handful of macros:
 *
 *  - `PAGE_TABLE_DEPTH`: depth of the address translation structure
 *  - `DMAP_BASE`: virtual address of the beginning of DMAP
 *  - `MAX_ASID`: maximum possible ASID
 *  - `PTE_EMPTY_KERNEL`: PTE value used for kernel empty page table entries
 *  - `PTE_EMPTY_USER`: PTE value used for user empty page table entries
 *  - `PTE_SET_ON_REFERENCED`: PTE bits to set while marking a page
 *    as referenced
 *  - `PTE_CLR_ON_REFERENCED`: PTE bits to clear while marking a page
 *    as referenced
 *  - `PTE_SET_ON_MODIFIED`: PTE bits to set while marking a page as modified
 *  - `PTE_CLR_ON_MODIFIED`: PTE bits to clear while marking a page as modified
 *  - `GROWKERNEL_STRIDE`: stride used while expanding the kernel virtual
 *    address space
 *
 * Besides, before the pmap module can be used, each target must:
 *
 *  - build DMAP and set the `dmap_paddr_base` and `dmap_paddr_end`
 *  - set the base address of the kernel page directory (`kernel_pde`)
 *
 * The pmap module requires the following machine-dependent types to be defined:
 *
 *  - `pte_t`: page table entry
 *  - `pde_t`: page directory entry
 *  - `asid_t`: address space identifier
 */

extern paddr_t dmap_paddr_base;
extern paddr_t dmap_paddr_end;

/*
 * Page directory.
 */

static inline bool pde_valid_p(pde_t *pdep);
paddr_t pde_alloc(pmap_t *pmap);
pde_t pde_make(int lvl, paddr_t pa);

static __no_profile inline pde_t *pde_ptr_idx(paddr_t pd_pa, size_t index) {
  pde_t *pde = phys_to_dmap(pd_pa);
  return pde + index;
}

static __no_profile inline pde_t *pde_ptr(paddr_t pd_pa, int lvl, vaddr_t va) {
  return pde_ptr_idx(pd_pa, pde_index(lvl, va));
}

/*
 * Page table.
 */

static inline paddr_t pte_frame(pte_t pte);
static inline bool pte_valid_p(pte_t *ptep);
static inline bool pte_access(pte_t pte, vm_prot_t prot);
pte_t pte_make(paddr_t pa, vm_prot_t prot, unsigned flags);
pte_t pte_protect(pte_t pte, vm_prot_t prot);

/*
 * Physical map management.
 */

void pmap_md_bootstrap(pde_t *pd);
void pmap_md_setup(pmap_t *pmap);
void pmap_md_activate(pmap_t *pmap);
void pmap_md_delete(pmap_t *pmap);
void pmap_md_growkernel(vaddr_t old_kva, vaddr_t new_kva);
void pmap_md_update(pmap_t *pmap);

/*
 * Direct map.
 */
void *phys_to_dmap(paddr_t addr);

#endif /* !_SYS__PMAP_H_ */
