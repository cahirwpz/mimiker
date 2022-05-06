#ifndef _SYS__PMAP_H_
#define _SYS__PMAP_H_

#ifndef _MACHDEP
#error "Do not use this header file outside kernel machine dependent code!"
#endif

#include <sys/mimiker.h>
#include <sys/mutex.h>
#include <sys/vm.h>
#include <machine/pmap.h>

typedef struct pmap {
  mtx_t mtx;                      /* protects all fields in this structure */
  asid_t asid;                    /* address space identifier */
  paddr_t pde;                    /* directory page table physical address */
  vm_pagelist_t pte_pages;        /* pages we allocate in page table */
  TAILQ_HEAD(, pv_entry) pv_list; /* all pages mapped by this physical map */

  /* Machine-dependent part */
  PMAP_MD_FIELDS;
} pmap_t;

typedef struct pv_entry {
  TAILQ_ENTRY(pv_entry) pmap_link; /* link on `pmap::pv_list` */
  TAILQ_ENTRY(pv_entry) page_link; /* link on `vm_page::pv_list` */
  pmap_t *pmap;                    /* page is mapped in this pmap */
  vaddr_t va;                      /* under this address */
} pv_entry_t;

/*
 * Each target must implement the following interface
 * along with a handful of macros:
 *
 *  - `ADDR_TRANSLATION_DEPTH`: depth of the address translation structure
 *  - `PMAP_MD_FIELDS`: machine-dependent fields of the pmap structure
 *  - `DMAP_BASE`: virtual address of the beginning of DMAP
 *  - `MAX_ASID`: maximum possible ASID
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

extern paddr_t kernel_pde;

/*
 * Translation structure.
 */

size_t l0_index(vaddr_t va);
size_t l1_index(vaddr_t va);
#if ADDR_TRANSLATION_DEPTH > 2
size_t l2_index(vaddr_t va);
#if ADDR_TRANSLATION_DEPTH == 4
size_t l3_index(vaddr_t va);
#endif
#endif

/*
 * Page directory.
 */

bool pde_valid_p(pde_t pde);
pde_t pde_make(paddr_t pa, unsigned lvl);
paddr_t pde2pa(pde_t pde);

void kernel_pd_change_notif(pmap_t *pmap, vaddr_t va, pde_t pde);

/*
 * Page table.
 */

bool pte_valid_p(pte_t pte);
bool pte_readable(pte_t pte);
bool pte_writable(pte_t pte);
bool pte_executable(pte_t pte);
pte_t pte_make(paddr_t pa, vm_prot_t prot, unsigned flags, bool kernel);
pte_t pte_protect(pte_t pte, vm_prot_t prot);
paddr_t pte2pa(pte_t pte);

/*
 * Physical map management.
 */

void pmap_md_setup(pmap_t *pmap);
void pmap_md_activate(pmap_t *pmap);
void pmap_md_delete(pmap_t *pmap);

#endif /* !_SYS__PMAP_H_ */
