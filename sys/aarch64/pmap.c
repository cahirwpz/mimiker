#define KL_LOG KL_PMAP
#include <sys/klog.h>
#include <sys/mimiker.h>
#include <sys/libkern.h>
#include <sys/pool.h>
#include <aarch64/pte.h>
#include <aarch64/tlb.h>
#include <aarch64/pmap.h>
#include <sys/pcpu.h>
#include <sys/pmap.h>
#include <sys/mutex.h>
#include <sys/sched.h>
#include <sys/vm_physmem.h>
#include <bitstring.h>

typedef struct pmap {
  mtx_t mtx;                      /* protects all fields in this structure */
  asid_t asid;                    /* address space identifier */
  pde_t *pde;                     /* directory page table (dmap) */
  vm_pagelist_t pte_pages;        /* pages we allocate in page table */
  TAILQ_HEAD(, pv_entry) pv_list; /* all pages mapped by this physical map */
} pmap_t;

typedef struct pv_entry {
  TAILQ_ENTRY(pv_entry) pmap_link; /* link on pmap::pv_list */
  TAILQ_ENTRY(pv_entry) page_link; /* link on vm_page::pv_list */
  pmap_t *pmap;                    /* page is mapped in this pmap */
  vaddr_t va;                      /* under this address */
} pv_entry_t;

static POOL_DEFINE(P_PMAP, "pmap", sizeof(pmap_t));
static POOL_DEFINE(P_PV, "pv_entry", sizeof(pv_entry_t));

#define ADDR_MASK 0x8ffffffff000
#define DMAP_BASE 0xffffff8000000000 /* last 512GB */
#define PHYS_TO_DMAP(x) ((intptr_t)(x) + DMAP_BASE)
#define PG_DMAP_ADDR(pg) ((void *)((intptr_t)(pg)->paddr + DMAP_BASE))

static const pte_t pte_default = L3_PAGE | ATTR_AF | ATTR_SH(ATTR_SH_IS);

static const pte_t vm_prot_map[] = {
  [VM_PROT_NONE] = ATTR_XN | pte_default,
  [VM_PROT_READ] = ATTR_AP(ATTR_AP_RO) | ATTR_XN | pte_default,
  [VM_PROT_WRITE] = ATTR_AP(ATTR_AP_RW) | ATTR_XN | pte_default,
  [VM_PROT_READ | VM_PROT_WRITE] = ATTR_AP(ATTR_AP_RW) | ATTR_XN | pte_default,
  [VM_PROT_EXEC] = pte_default,
  [VM_PROT_READ | VM_PROT_EXEC] = ATTR_AP(ATTR_AP_RO) | pte_default,
  [VM_PROT_WRITE | VM_PROT_EXEC] = ATTR_AP(ATTR_AP_RW) | pte_default,
  [VM_PROT_READ | VM_PROT_WRITE | VM_PROT_EXEC] =
    ATTR_AP(ATTR_AP_RW) | pte_default,
};

static pmap_t kernel_pmap;
/* Kernel page directory entries. */
alignas(PAGESIZE) pde_t _kernel_pmap_pde[PD_ENTRIES];
static bitstr_t asid_used[bitstr_size(MAX_ASID)] = {0};
static spin_t *asid_lock = &SPIN_INITIALIZER(0);

/*
 * Helper functions.
 */
static bool user_addr_p(vaddr_t addr) {
  return (addr >= PMAP_USER_BEGIN) && (addr < PMAP_USER_END);
}

static bool kern_addr_p(vaddr_t addr) {
  return (addr >= PMAP_KERNEL_BEGIN) && (addr < PMAP_KERNEL_END);
}

inline vaddr_t pmap_start(pmap_t *pmap) {
  return pmap->asid ? PMAP_USER_BEGIN : PMAP_KERNEL_BEGIN;
}

inline vaddr_t pmap_end(pmap_t *pmap) {
  return pmap->asid ? PMAP_USER_END : PMAP_KERNEL_END;
}

inline bool pmap_address_p(pmap_t *pmap, vaddr_t va) {
  return pmap_start(pmap) <= va && va < pmap_end(pmap);
}

inline bool pmap_contains_p(pmap_t *pmap, vaddr_t start, vaddr_t end) {
  return pmap_start(pmap) <= start && end <= pmap_end(pmap);
}

inline pmap_t *pmap_kernel(void) {
  return &kernel_pmap;
}

inline pmap_t *pmap_user(void) {
  return PCPU_GET(curpmap);
}

pmap_t *pmap_lookup(vaddr_t addr) {
  if (kern_addr_p(addr))
    return pmap_kernel();
  if (user_addr_p(addr))
    return pmap_user();
  return NULL;
}

/*
 * Address space identifiers management.
 */

static asid_t alloc_asid(void) {
  int free = 0;
  WITH_SPIN_LOCK (asid_lock) {
    bit_ffc(asid_used, MAX_ASID, &free);
    if (free < 0)
      panic("Out of asids!");
    bit_set(asid_used, free);
  }
  klog("alloc_asid() = %d", free);
  return free;
}

static void free_asid(asid_t asid) {
  klog("free_asid(%d)", asid);
  SCOPED_SPIN_LOCK(asid_lock);
  bit_clear(asid_used, (unsigned)asid);
  tlb_invalidate_asid(asid);
}

/*
 * Physical-to-virtual entries are managed for all pageable mappings.
 */

static void pv_add(pmap_t *pmap, vaddr_t va, vm_page_t *pg) {
  pv_entry_t *pv = pool_alloc(P_PV, M_ZERO);
  pv->pmap = pmap;
  pv->va = va;
  TAILQ_INSERT_TAIL(&pg->pv_list, pv, page_link);
  TAILQ_INSERT_TAIL(&pmap->pv_list, pv, pmap_link);
}

static pv_entry_t *pv_find(pmap_t *pmap, vaddr_t va, vm_page_t *pg) {
  pv_entry_t *pv;
  TAILQ_FOREACH (pv, &pg->pv_list, page_link) {
    if (pv->pmap == pmap && pv->va == va)
      return pv;
  }
  return NULL;
}

static void pv_remove(pmap_t *pmap, vaddr_t va, vm_page_t *pg) {
  pv_entry_t *pv = pv_find(pmap, va, pg);
  assert(pv != NULL);
  TAILQ_REMOVE(&pg->pv_list, pv, page_link);
  TAILQ_REMOVE(&pmap->pv_list, pv, pmap_link);
  pool_free(P_PV, pv);
}

/*
 * Routines for accessing page table entries.
 */

static vm_page_t *pmap_pagealloc(void) {
  vm_page_t *pg = vm_page_alloc(1);
  pmap_zero_page(pg);
  return pg;
}

static pde_t *pmap_l0(pmap_t *pmap, vaddr_t va) {
  return &pmap->pde[L0_INDEX(va)];
}

static pde_t *pmap_l1(pmap_t *pmap, vaddr_t va) {
  pde_t l1 = *pmap_l0(pmap, va) & ADDR_MASK;
  pde_t *dmap_l1 = (pde_t *)PHYS_TO_DMAP(l1);
  return &dmap_l1[L1_INDEX(va)];
}

static pde_t *pmap_l2(pmap_t *pmap, vaddr_t va) {
  pde_t l2 = *pmap_l1(pmap, va) & ADDR_MASK;
  pde_t *dmap_l2 = (pde_t *)PHYS_TO_DMAP(l2);
  return &dmap_l2[L2_INDEX(va)];
}

static pte_t *pmap_l3(pmap_t *pmap, vaddr_t va) {
  pte_t l3 = *pmap_l2(pmap, va) & ADDR_MASK;
  pte_t *dmap_l3 = (pte_t *)PHYS_TO_DMAP(l3);
  return &dmap_l3[L3_INDEX(va)];
}

static pde_t *pmap_alloc_pde(pmap_t *pmap, vaddr_t vaddr, int level) {
  vm_page_t *pg = pmap_pagealloc();
  pde_t *pde = (pde_t *)PHYS_TO_DMAP(pg->paddr);

  if (level == 3)
    TAILQ_INSERT_TAIL(&pmap->pte_pages, pg, pageq);

  klog("Page table level %d for 0x%016lx allocated at 0x%016lx", level, vaddr,
       pg->paddr);

  for (int i = 0; i < PD_ENTRIES; ++i)
    pde[i] = 0;

  return (pde_t *)pg->paddr;
}

static void pmap_pte_write(pmap_t *pmap, pte_t *ptep, pte_t pte,
                           unsigned flags) {
  unsigned cacheflags = flags & PMAP_CACHE_MASK;

  if (cacheflags == PMAP_NOCACHE)
    pte |= ATTR_IDX(ATTR_NORMAL_MEM_NC);
  else if (cacheflags == PMAP_WRITE_THROUGH)
    pte |= ATTR_IDX(ATTR_NORMAL_MEM_WT);
  else
    pte |= ATTR_IDX(ATTR_NORMAL_MEM_WB);

  *ptep = pte;

  tlb_invalidate(pte, pmap->asid);
}

/*
 * Return pointer to entry of va in level 3 of page table. Allocate space if
 * needed.
 */
static pte_t *pmap_ensure_pte(pmap_t *pmap, vaddr_t va) {
  pde_t *l0 = pmap_l0(pmap, va);
  if (*l0 == 0)
    *l0 = (pde_t)pmap_alloc_pde(pmap, va, 1) | L0_TABLE;

  pde_t *l1 = pmap_l1(pmap, va);
  if (*l1 == 0)
    *l1 = (pde_t)pmap_alloc_pde(pmap, va, 2) | L1_TABLE;

  pde_t *l2 = pmap_l2(pmap, va);
  if (*l2 == 0)
    *l2 = (pde_t)pmap_alloc_pde(pmap, va, 3) | L2_TABLE;

  return pmap_l3(pmap, va);
}

void pmap_activate(pmap_t *pmap) {
  SCOPED_NO_PREEMPTION();

  if (pmap != pmap_kernel()) {
    panic("Not implemented!");
  }

  PCPU_SET(curpmap, pmap);
}

/*
 * Wired memory interface.
 */

void pmap_kenter(vaddr_t va, paddr_t pa, vm_prot_t prot, unsigned flags) {
  pmap_t *pmap = pmap_kernel();

  assert(page_aligned_p(pa) && page_aligned_p(va));
  assert(pmap_address_p(pmap, va));
  assert(pa != 0);

  klog("Enter unmanaged mapping from %p to %p", va, pa);

  WITH_MTX_LOCK (&pmap->mtx) {
    pte_t *l3 = pmap_ensure_pte(pmap, va);
    pmap_pte_write(pmap, l3, pa | vm_prot_map[prot], flags);
  }
}

void pmap_kremove(vaddr_t va, size_t size) {
  panic("Not implemented!");
}

bool pmap_kextract(vaddr_t va, paddr_t *pap) {
  return pmap_extract(pmap_kernel(), va, pap);
}

/*
 * Pageable (user & kernel) memory interface.
 */

static bool pmap_extract_nolock(pmap_t *pmap, vaddr_t va, paddr_t *pap) {
  if (!pmap_address_p(pmap, va))
    return false;

  pde_t *l0 = pmap_l0(pmap, va);
  if (*l0 == 0)
    return false;

  pde_t *l1 = pmap_l1(pmap, va);
  if (*l1 == 0)
    return false;

  pde_t *l2 = pmap_l2(pmap, va);
  if (*l2 == 0)
    return false;

  pte_t *l3 = pmap_l3(pmap, va);
  if (*l3 == 0)
    return false;

  /* TODO(pj) make it readable */
  *pap = (*l3 & 0xfffffffff000) | (va & (PAGESIZE - 1));
  return true;
}

void pmap_enter(pmap_t *pmap, vaddr_t va, vm_page_t *pg, vm_prot_t prot,
                unsigned flags) {
  paddr_t pa = pg->paddr;

  assert(page_aligned_p(va));
  assert(pmap_address_p(pmap, pa));

  klog("Enter virtual mapping %p for frame %p", va, pa);

  bool kern_mapping = (pmap == pmap_kernel());

  /* TODO(pj) Mark user pages as non-referenced & non-modified. */
  pte_t pte = vm_prot_map[prot];

  WITH_MTX_LOCK (&pmap->mtx) {
    pv_entry_t *pv = pv_find(pmap, va, pg);
    if (pv == NULL)
      pv_add(pmap, va, pg);
    if (kern_mapping)
      pg->flags |= PG_MODIFIED | PG_REFERENCED;
    else
      pg->flags &= ~(PG_MODIFIED | PG_REFERENCED);
    pte_t *l3 = pmap_ensure_pte(pmap, va);
    pmap_pte_write(pmap, l3, pa | pte, flags);
  }
}

void pmap_remove(pmap_t *pmap, vaddr_t start, vaddr_t end) {
  assert(page_aligned_p(start) && page_aligned_p(end) && start < end);
  assert(pmap_contains_p(pmap, start, end));

  klog("Remove page mapping for address range %p-%p", start, end);

  WITH_MTX_LOCK (&pmap->mtx) {
    for (vaddr_t va = start; va < end; va += PAGESIZE) {
      paddr_t pa;
      if (pmap_extract_nolock(pmap, va, &pa)) {
        vm_page_t *pg = vm_page_find(pa);
        pv_remove(pmap, va, pg);
        pte_t *l3 = pmap_l3(pmap, va);
        pmap_pte_write(pmap, l3, 0, 0);
      }
    }
  }
}

void pmap_protect(pmap_t *pmap, vaddr_t start, vaddr_t end, vm_prot_t prot) {
  assert(page_aligned_p(start) && page_aligned_p(end) && start < end);
  assert(pmap_contains_p(pmap, start, end));

  klog("Change protection bits to %x for address range %p-%p", prot, start,
       end);

  WITH_MTX_LOCK (&pmap->mtx) {
    for (vaddr_t va = start; va < end; va += PAGESIZE) {
      pte_t *l3 = pmap_l3(pmap, va);
      pmap_pte_write(pmap, l3,
                     (*l3 & (~ATTR_AP_MASK & ~ATTR_XN)) | vm_prot_map[prot], 0);
    }
  }
}

bool pmap_extract(pmap_t *pmap, vaddr_t va, paddr_t *pap) {
  SCOPED_MTX_LOCK(&pmap->mtx);
  return pmap_extract_nolock(pmap, va, pap);
}

void pmap_page_remove(vm_page_t *pg) {
  while (!TAILQ_EMPTY(&pg->pv_list)) {
    pv_entry_t *pv = TAILQ_FIRST(&pg->pv_list);
    pmap_t *pmap = pv->pmap;
    vaddr_t va = pv->va;
    TAILQ_REMOVE(&pg->pv_list, pv, page_link);
    TAILQ_REMOVE(&pmap->pv_list, pv, pmap_link);
    pte_t *pte = pmap_l3(pmap, va);
    pmap_pte_write(pmap, pte, 0, 0);
    pool_free(P_PV, pv);
  }
}

void pmap_zero_page(vm_page_t *pg) {
  bzero(PG_DMAP_ADDR(pg), PAGESIZE);
}

void pmap_copy_page(vm_page_t *src, vm_page_t *dst) {
  memcpy(PG_DMAP_ADDR(src), PG_DMAP_ADDR(dst), PAGESIZE);
}

static void pmap_modify_flags(vm_page_t *pg, pte_t set, pte_t clr) {
  panic("Not implemented!");
}

bool pmap_clear_referenced(vm_page_t *pg) {
  bool prev = pmap_is_referenced(pg);
  pg->flags &= ~PG_REFERENCED;
  pmap_modify_flags(pg, 0, 0 /* PTE_VALID */);
  return prev;
}

bool pmap_clear_modified(vm_page_t *pg) {
  bool prev = pmap_is_modified(pg);
  pg->flags &= ~PG_MODIFIED;
  pmap_modify_flags(pg, 0, 0 /* PTE_DIRTY */);
  return prev;
}

bool pmap_is_referenced(vm_page_t *pg) {
  return pg->flags & PG_REFERENCED;
}

bool pmap_is_modified(vm_page_t *pg) {
  return pg->flags & PG_MODIFIED;
}

void pmap_set_referenced(vm_page_t *pg) {
  pg->flags |= PG_REFERENCED;
  pmap_modify_flags(pg, 0 /* PTE_VALID */, 0);
}

void pmap_set_modified(vm_page_t *pg) {
  pg->flags |= PG_MODIFIED;
  pmap_modify_flags(pg, 0 /* PTE_DIRTY */, 0);
}

/*
 * Physical map management routines.
 */

static void pmap_setup(pmap_t *pmap) {
  pmap->asid = alloc_asid();
  mtx_init(&pmap->mtx, 0);
  TAILQ_INIT(&pmap->pte_pages);
  TAILQ_INIT(&pmap->pv_list);
}

void init_pmap(void) {
  pmap_setup(&kernel_pmap);
  kernel_pmap.pde = _kernel_pmap_pde;
}

pmap_t *pmap_new(void) {
  pmap_t *pmap = pool_alloc(P_PMAP, M_ZERO);
  pmap_setup(pmap);
  return pmap;
}

void pmap_delete(pmap_t *pmap) {
  assert(pmap != pmap_kernel());

  while (!TAILQ_EMPTY(&pmap->pv_list)) {
    pv_entry_t *pv = TAILQ_FIRST(&pmap->pv_list);
    vm_page_t *pg;
    paddr_t pa;
    pmap_extract(pmap, pv->va, &pa);
    pg = vm_page_find(pa);
    TAILQ_REMOVE(&pg->pv_list, pv, page_link);
    TAILQ_REMOVE(&pmap->pv_list, pv, pmap_link);
    pool_free(P_PV, pv);
  }

  while (!TAILQ_EMPTY(&pmap->pte_pages)) {
    vm_page_t *pg = TAILQ_FIRST(&pmap->pte_pages);
    vm_page_free(pg);
    free_asid(pmap->asid);
  }

  pool_free(P_PMAP, pmap);
}
