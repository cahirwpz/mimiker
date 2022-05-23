#define KL_LOG KL_PMAP
#include <bitstring.h>
#include <sys/errno.h>
#include <sys/kasan.h>
#include <sys/klog.h>
#include <sys/libkern.h>
#include <sys/pcpu.h>
#include <sys/pool.h>
#include <sys/pmap.h>
#include <sys/sched.h>
#include <sys/vm_physmem.h>
#include <sys/_pmap.h>
#include <sys/_tlb.h>

#define PAGE_OFFSET(x) ((x) & (PAGESIZE - 1))

static_assert(PAGE_TABLE_DEPTH, "Page table depth defined to 0!");

static POOL_DEFINE(P_PMAP, "pmap", sizeof(pmap_t));
static POOL_DEFINE(P_PV, "pv_entry", sizeof(pv_entry_t));

/* Physical memory boundaries. */
paddr_t dmap_paddr_base;
paddr_t dmap_paddr_end;

paddr_t kernel_pde;
static pmap_t kernel_pmap;

/* Bitmap of used ASIDs. */
static bitstr_t asid_used[bitstr_size(MAX_ASID)] = {0};
static SPIN_DEFINE(asid_lock, 0);

/*
 * This lock is used to protect the `vm_page::pv_list` field.
 * Order of acquiring locks:
 *  1. `pv_list_lock`
 *  2. `pmap_t::mtx`
 */
static MTX_DEFINE(pv_list_lock, 0);

/*
 * Some architectures provide a separate active page directory for kernel
 * address space, whereas other architecutres use a single translation structure
 * for both kernel and user addresses. In such architecures, the physical map
 * of a user process must contain the entire page directory of the kernel.
 * This requires broadcasting any changes in the top page directory
 * of the kernel among all user physical maps.
 *
 * Order of acquiring locks:
 *  - `kernel_pmap->mtx`
 *  - `user_pmaps_lock`
 */
#if SHARED_KERNEL_PD
static MTX_DEFINE(user_pmaps_lock, 0);
static LIST_HEAD(, pmap) user_pmaps = LIST_HEAD_INITIALIZER(user_pmaps);
#endif /* !SHARED_KERNEL_PD */

/*
 * Helper functions.
 */

static bool user_addr_p(vaddr_t addr) {
  return addr >= USER_SPACE_BEGIN && addr < USER_SPACE_END;
}

static bool kern_addr_p(vaddr_t addr) {
  return addr >= KERNEL_SPACE_BEGIN && addr < KERNEL_SPACE_END;
}

static vaddr_t pmap_start(pmap_t *pmap) {
  return pmap->asid ? USER_SPACE_BEGIN : KERNEL_SPACE_BEGIN;
}

static vaddr_t pmap_end(pmap_t *pmap) {
  return pmap->asid ? USER_SPACE_END : KERNEL_SPACE_END;
}

static bool pmap_address_p(pmap_t *pmap, vaddr_t va) {
  return va >= pmap_start(pmap) && va < pmap_end(pmap);
}

static bool pmap_contains_p(pmap_t *pmap, vaddr_t start, vaddr_t end) {
  return start >= pmap_start(pmap) && end <= pmap_end(pmap);
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

vaddr_t phys_to_dmap(paddr_t addr) {
  /* TODO: remove the following `if` and uncomment the assertion
   * that follows when MIPS support is finally removed.
   * assert(addr >= dmap_paddr_base && addr < dmap_paddr_end); */
  if (addr >= DMAP_BASE)
    return (vaddr_t)addr;
  return (vaddr_t)(addr - dmap_paddr_base) + DMAP_BASE;
}

static vaddr_t pg_dmap_addr(vm_page_t *pg) {
  return phys_to_dmap(pg->paddr);
}

static vm_page_t *pmap_pagealloc(void) {
  vm_page_t *pg = vm_page_alloc(1);
  pmap_zero_page(pg);
  return pg;
}

void pmap_zero_page(vm_page_t *pg) {
  memset((void *)pg_dmap_addr(pg), 0, PAGESIZE);
}

void pmap_copy_page(vm_page_t *src, vm_page_t *dst) {
  memcpy((void *)pg_dmap_addr(dst), (void *)pg_dmap_addr(src), PAGESIZE);
}

/*
 * Address space identifiers management.
 */

static asid_t alloc_asid(void) {
  int free = 0;
  WITH_SPIN_LOCK (&asid_lock) {
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
  SCOPED_SPIN_LOCK(&asid_lock);
  bit_clear(asid_used, (unsigned)asid);
  tlb_invalidate_asid(asid);
}

/*
 * Physical-to-virtual entries are managed for all pageable mappings.
 */

static void pv_add(pmap_t *pmap, vaddr_t va, vm_page_t *pg) {
  assert(mtx_owned(&pv_list_lock));
  pv_entry_t *pv = pool_alloc(P_PV, M_ZERO);
  pv->pmap = pmap;
  pv->va = va;
  TAILQ_INSERT_TAIL(&pg->pv_list, pv, page_link);
  TAILQ_INSERT_TAIL(&pmap->pv_list, pv, pmap_link);
}

static pv_entry_t *pv_find(pmap_t *pmap, vaddr_t va, vm_page_t *pg) {
  assert(mtx_owned(&pv_list_lock));
  pv_entry_t *pv;
  TAILQ_FOREACH (pv, &pg->pv_list, page_link) {
    if (pv->pmap == pmap && pv->va == va)
      return pv;
  }
  return NULL;
}

static void pv_remove(pmap_t *pmap, vaddr_t va, vm_page_t *pg) {
  assert(mtx_owned(&pv_list_lock));
  pv_entry_t *pv = pv_find(pmap, va, pg);
  TAILQ_REMOVE(&pg->pv_list, pv, page_link);
  TAILQ_REMOVE(&pmap->pv_list, pv, pmap_link);
  pool_free(P_PV, pv);
}

/*
 * Routines for accessing page table entries.
 */

static paddr_t pmap_alloc_pde(pmap_t *pmap, vaddr_t va) {
  assert(mtx_owned(&pmap->mtx));

  vm_page_t *pg = pmap_pagealloc();

  TAILQ_INSERT_TAIL(&pmap->pte_pages, pg, pageq);

  klog("Page table for %p allocated at %p", (void *)va, pg->paddr);

  return pg->paddr;
}

static pte_t *pmap_lookup_pte(pmap_t *pmap, vaddr_t va) {
  pde_t *pdep = (pde_t *)phys_to_dmap(pmap->pde) + pt_index(0, va);

  for (unsigned lvl = 1; lvl < PAGE_TABLE_DEPTH; lvl++) {
    if (!pde_valid_p(*pdep))
      return NULL;
    paddr_t pa = pte_frame((pte_t)*pdep);
    pdep = (pde_t *)phys_to_dmap(pa) + pt_index(lvl, va);
  }

  return (pte_t *)pdep;
}

static void pmap_write_pte(pmap_t *pmap, pte_t *ptep, pte_t pte, vaddr_t va) {
  *ptep = pte;
  tlb_invalidate(va, pmap->asid);
}

#if SHARED_KERNEL_PD
static void broadcast_kernel_top_pde(unsigned idx, pde_t pde) {
  SCOPED_MTX_LOCK(&user_pmaps_lock);

  pmap_t *user_pmap;
  LIST_FOREACH (user_pmap, &user_pmaps, pmap_link) {
    pde_t *pdep = (pde_t *)phys_to_dmap(user_pmap->pde);
    pdep[idx] = pde;
  }
}
#endif /* !SHARED_KERNEL_PD */

/* Return PTE pointer for `va`. Allocate page table if needed. */
static pte_t *pmap_ensure_pte(pmap_t *pmap, vaddr_t va) {
  assert(mtx_owned(&pmap->mtx));

  pde_t *pdep = (pde_t *)phys_to_dmap(pmap->pde) + pt_index(0, va);

  for (unsigned lvl = 0; lvl < PAGE_TABLE_DEPTH - 1; lvl++) {
    paddr_t pa;
    if (!pde_valid_p(*pdep)) {
      pa = pmap_alloc_pde(pmap, va);
      *pdep = pde_make(lvl, pa);
#if SHARED_KERNEL_PD
      if (lvl == 0 && pmap == pmap_kernel())
        broadcast_kernel_top_pde(pt_index(0, va), *pdep);
#endif /* !SHARED_KERNEL_PD */
    } else {
      pa = pte_frame((pte_t)*pdep);
    }
    pdep = (pde_t *)phys_to_dmap(pa) + pt_index(lvl + 1, va);
  }

  return (pte_t *)pdep;
}

/*
 * Wired memory interface.
 */

void pmap_kenter(vaddr_t va, paddr_t pa, vm_prot_t prot, unsigned flags) {
  pmap_t *pmap = pmap_kernel();

  assert(page_aligned_p(pa) && page_aligned_p(va));
  assert(pmap_address_p(pmap, va));

  klog("Enter unmanaged mapping from %p to %p", va, pa);

  pte_t pte = pte_make(pa, prot, flags, true);

  WITH_MTX_LOCK (&pmap->mtx) {
    pte_t *ptep = pmap_ensure_pte(pmap, va);
    pmap_write_pte(pmap, ptep, pte, va);
  }
}

void pmap_kremove(vaddr_t va, size_t size) {
  pmap_t *pmap = pmap_kernel();

  assert(page_aligned_p(va) && page_aligned_p(size));
  assert(pmap_contains_p(pmap, va, va + size));

  klog("Remove unmanaged mapping for %p - %p range", va, va + size - 1);

  WITH_MTX_LOCK (&pmap->mtx) {
    for (size_t off = 0; off < size; off += PAGESIZE) {
      pte_t *ptep = pmap_lookup_pte(pmap, va + off);
      assert(ptep);
      pmap_write_pte(pmap, ptep, pte_empty(true), va + off);
    }
  }
}

bool pmap_kextract(vaddr_t va, paddr_t *pap) {
  return pmap_extract(pmap_kernel(), va, pap);
}

/*
 * Pageable user memory interface.
 */

void pmap_enter(pmap_t *pmap, vaddr_t va, vm_page_t *pg, vm_prot_t prot,
                unsigned flags) {
  assert(pmap != pmap_kernel());

  paddr_t pa = pg->paddr;

  assert(page_aligned_p(va));
  assert(pmap_address_p(pmap, va));

  klog("Enter virtual mapping %p for frame %p", va, pa);

  pte_t pte = pte_make(pa, prot, flags, false);

  WITH_MTX_LOCK (&pv_list_lock) {
    WITH_MTX_LOCK (&pmap->mtx) {
      pv_entry_t *pv = pv_find(pmap, va, pg);
      if (!pv)
        pv_add(pmap, va, pg);
      pg->flags &= ~(PG_MODIFIED | PG_REFERENCED);
      pte_t *ptep = pmap_ensure_pte(pmap, va);
      pmap_write_pte(pmap, ptep, pte, va);
    }
  }
}

void pmap_remove(pmap_t *pmap, vaddr_t start, vaddr_t end) {
  assert(pmap != pmap_kernel());
  assert(page_aligned_p(start) && page_aligned_p(end) && start < end);
  assert(pmap_contains_p(pmap, start, end));

  klog("Remove page mapping for address range %p - %p", start, end);

  WITH_MTX_LOCK (&pv_list_lock) {
    WITH_MTX_LOCK (&pmap->mtx) {
      for (vaddr_t va = start; va < end; va += PAGESIZE) {
        pte_t *ptep = pmap_lookup_pte(pmap, va);
        if (!ptep || !pte_valid_p(*ptep))
          continue;
        paddr_t pa = pte_frame(*ptep);
        vm_page_t *pg = vm_page_find(pa);
        pv_remove(pmap, va, pg);
        pmap_write_pte(pmap, ptep, pte_empty(false), va);
      }
    }
  }
}

static bool pmap_extract_nolock(pmap_t *pmap, vaddr_t va, paddr_t *pap) {
  if (!pmap_address_p(pmap, va))
    return false;

  pte_t *ptep = pmap_lookup_pte(pmap, va);
  if (ptep == NULL || !pte_valid_p(*ptep))
    return false;

  paddr_t pa = pte_frame(*ptep);
  *pap = pa | PAGE_OFFSET(va);
  return true;
}

bool pmap_extract(pmap_t *pmap, vaddr_t va, paddr_t *pap) {
  SCOPED_MTX_LOCK(&pmap->mtx);
  return pmap_extract_nolock(pmap, va, pap);
}

void pmap_protect(pmap_t *pmap, vaddr_t start, vaddr_t end, vm_prot_t prot) {
  assert(pmap != pmap_kernel());
  assert(page_aligned_p(start) && page_aligned_p(end) && start < end);
  assert(pmap_contains_p(pmap, start, end));

  klog("Change protection bits to %x for address range %p - %p", prot, start,
       end);

  WITH_MTX_LOCK (&pmap->mtx) {
    for (vaddr_t va = start; va < end; va += PAGESIZE) {
      pte_t *ptep = pmap_lookup_pte(pmap, va);
      if (!ptep || !pte_valid_p(*ptep))
        continue;
      pte_t pte = pte_protect(*ptep, prot);
      pmap_write_pte(pmap, ptep, pte, va);
    }
  }
}

void pmap_page_remove(vm_page_t *pg) {
  SCOPED_MTX_LOCK(&pv_list_lock);

  while (!TAILQ_EMPTY(&pg->pv_list)) {
    pv_entry_t *pv = TAILQ_FIRST(&pg->pv_list);
    pmap_t *pmap = pv->pmap;
    assert(pmap != pmap_kernel());
    vaddr_t va = pv->va;
    WITH_MTX_LOCK (&pmap->mtx) {
      TAILQ_REMOVE(&pg->pv_list, pv, page_link);
      TAILQ_REMOVE(&pmap->pv_list, pv, pmap_link);
      pte_t *ptep = pmap_lookup_pte(pmap, va);
      assert(ptep);
      pmap_write_pte(pmap, ptep, pte_empty(false), va);
    }
    pool_free(P_PV, pv);
  }
}

static void pmap_modify_flags(vm_page_t *pg, pte_t set, pte_t clr) {
  SCOPED_MTX_LOCK(&pv_list_lock);

  pv_entry_t *pv;
  TAILQ_FOREACH (pv, &pg->pv_list, page_link) {
    pmap_t *pmap = pv->pmap;
    assert(pmap != pmap_kernel());
    vaddr_t va = pv->va;
    WITH_MTX_LOCK (&pmap->mtx) {
      pte_t *ptep = pmap_lookup_pte(pmap, va);
      assert(ptep);
      pte_t pte = *ptep;
      pte |= set;
      pte &= ~clr;
      pmap_write_pte(pmap, ptep, pte, va);
    }
  }
}

bool pmap_is_referenced(vm_page_t *pg) {
  return pg->flags & PG_REFERENCED;
}

bool pmap_is_modified(vm_page_t *pg) {
  return pg->flags & PG_MODIFIED;
}

void pmap_set_referenced(vm_page_t *pg) {
  pg->flags |= PG_REFERENCED;
  pmap_modify_flags(pg, PTE_SET_ON_REFERENCED, PTE_CLR_ON_REFERENCED);
}

void pmap_set_modified(vm_page_t *pg) {
  pg->flags |= PG_MODIFIED;
  pmap_modify_flags(pg, PTE_SET_ON_MODIFIED, PTE_CLR_ON_MODIFIED);
}

bool pmap_clear_referenced(vm_page_t *pg) {
  bool prev = pmap_is_referenced(pg);
  pg->flags &= ~PG_REFERENCED;
  pmap_modify_flags(pg, PTE_CLR_ON_REFERENCED, PTE_SET_ON_REFERENCED);
  return prev;
}

bool pmap_clear_modified(vm_page_t *pg) {
  bool prev = pmap_is_modified(pg);
  pg->flags &= ~PG_MODIFIED;
  pmap_modify_flags(pg, PTE_CLR_ON_MODIFIED, PTE_SET_ON_MODIFIED);
  return prev;
}

int pmap_emulate_bits(pmap_t *pmap, vaddr_t va, vm_prot_t prot) {
  paddr_t pa;

  WITH_MTX_LOCK (&pmap->mtx) {
    if (!pmap_extract_nolock(pmap, va, &pa))
      return EFAULT;

    pte_t pte = *pmap_lookup_pte(pmap, va);

    if ((prot & VM_PROT_READ) && !pte_access(pte, VM_PROT_READ))
      return EACCES;

    if ((prot & VM_PROT_WRITE) && !pte_access(pte, VM_PROT_WRITE))
      return EACCES;

    if ((prot & VM_PROT_EXEC) && !pte_access(pte, VM_PROT_EXEC))
      return EACCES;
  }

  vm_page_t *pg = vm_page_find(pa);
  assert(pg);

  pmap_set_referenced(pg);
  if (prot & VM_PROT_WRITE)
    pmap_set_modified(pg);

  return 0;
}

/*
 * Physical map management routines.
 */

static void pmap_setup(pmap_t *pmap) {
  TAILQ_INIT(&pmap->pte_pages);
  if (pmap != pmap_kernel()) {
    vm_page_t *pg = pmap_pagealloc();
    pmap->pde = pg->paddr;
    TAILQ_INSERT_TAIL(&pmap->pte_pages, pg, pageq);
  }
  pmap->asid = alloc_asid();
  mtx_init(&pmap->mtx, 0);
  TAILQ_INIT(&pmap->pv_list);

#if SHARED_KERNEL_PD
  /* Install kernel pagetables. */
  pmap_t *kmap = pmap_kernel();
  size_t off = PAGESIZE / 2;
  WITH_MTX_LOCK (&kmap->mtx) {
    memcpy((void *)phys_to_dmap(pmap->pde) + off,
           (void *)phys_to_dmap(kmap->pde) + off, PAGESIZE / 2);
  }

  WITH_MTX_LOCK (&user_pmaps_lock)
    LIST_INSERT_HEAD(&user_pmaps, pmap, pmap_link);
#endif /* !SHARED_KERNEL_PD */

  pmap_md_setup(pmap);
}

void init_pmap(void) {
  kernel_pmap.pde = kernel_pde;
  pmap_setup(&kernel_pmap);
}

pmap_t *pmap_new(void) {
  pmap_t *pmap = pool_alloc(P_PMAP, M_ZERO);
  pmap_setup(pmap);

  klog("Page directory table allocated at %p", pmap->pde);

  return pmap;
}

void pmap_activate(pmap_t *pmap) {
  SCOPED_NO_PREEMPTION();

  pmap_t *old = PCPU_GET(curpmap);
  if (pmap == old)
    return;

  pmap_md_activate(pmap);

  PCPU_SET(curpmap, pmap);
}

void pmap_delete(pmap_t *pmap) {
  assert(pmap != pmap_kernel());

  pmap_md_delete(pmap);

#if SHARED_KERNEL_PD
  WITH_MTX_LOCK (&user_pmaps_lock)
    LIST_REMOVE(pmap, pmap_link);
#endif /* !SHARED_KERNEL_PD */

  while (!TAILQ_EMPTY(&pmap->pv_list)) {
    pv_entry_t *pv = TAILQ_FIRST(&pmap->pv_list);
    paddr_t pa;
    assert(pmap_extract_nolock(pmap, pv->va, &pa));
    vm_page_t *pg = vm_page_find(pa);
    WITH_MTX_LOCK (&pv_list_lock)
      TAILQ_REMOVE(&pg->pv_list, pv, page_link);
    TAILQ_REMOVE(&pmap->pv_list, pv, pmap_link);
    pool_free(P_PV, pv);
  }

  while (!TAILQ_EMPTY(&pmap->pte_pages)) {
    vm_page_t *pg = TAILQ_FIRST(&pmap->pte_pages);
    TAILQ_REMOVE(&pmap->pte_pages, pg, pageq);
    vm_page_free(pg);
  }

  free_asid(pmap->asid);
  pool_free(P_PMAP, pmap);
}

/*
 * Increase usable kernel virtual address space to at least `maxkvaddr`.
 * Allocate page tables if needed.
 */
void pmap_growkernel(vaddr_t maxkvaddr) {
  assert(mtx_owned(&vm_kernel_end_lock));
  assert(maxkvaddr > vm_kernel_end);

  maxkvaddr = roundup2(maxkvaddr, GROWKERNEL_STRIDE);
  pmap_t *pmap = pmap_kernel();

  WITH_MTX_LOCK (&pmap->mtx) {
    for (vaddr_t va = vm_kernel_end; va < maxkvaddr; va += GROWKERNEL_STRIDE)
      (void)pmap_ensure_pte(pmap, va);
  }

  /*
   * `kasan_grow` calls `pmap_kenter` which acquires `pmap->mtx`.
   * But we are under `vm_kernel_end_lock` from kmem so it's safe to call
   * `kasan_grow`.
   */
  kasan_grow(maxkvaddr);

  vm_kernel_end = maxkvaddr;
}
