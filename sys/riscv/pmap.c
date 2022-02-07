#define KL_LOG KL_PMAP
#include <bitstring.h>
#include <sys/errno.h>
#include <sys/kasan.h>
#include <sys/kenv.h>
#include <sys/klog.h>
#include <sys/libkern.h>
#include <sys/mimiker.h>
#include <sys/mutex.h>
#include <sys/pcpu.h>
#include <sys/pool.h>
#include <sys/pmap.h>
#include <sys/sched.h>
#include <sys/spinlock.h>
#include <sys/vm_physmem.h>
#include <riscv/pmap.h>
#include <riscv/riscvreg.h>
#include <riscv/tlb.h>
#include <riscv/vm_param.h>

#define __sfence_vma() __asm __volatile("sfence.vma" ::: "memory")

typedef struct pmap {
  mtx_t mtx;               /* protects all fields in this structure */
  asid_t asid;             /* address space identifier */
  paddr_t pde;             /* directory page table physical address */
  paddr_t satp;            /* supervisor address translation and protection */
  vm_pagelist_t pte_pages; /* pages we allocate in page table */
  LIST_ENTRY(pmap) pmap_link;     /* link on `user_pmaps` */
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

/*
 * The following table describes which access bits need to be set in page table
 * entry for successful memory translation by MMU. Other configurations cause
 * memory fault - see riscv/trap.c.
 *
 * +--------------+---+---+------+---+---+---+---+
 * |    access    | D | A | USER | X | W | R | V |
 * +==============+===+===+======+===+===+===+===+
 * | user read    | * | 1 | 1    | * | * | 1 | 1 |
 * +--------------+---+---+------+---+---+---+---+
 * | user write   | 1 | 1 | 1    | * | 1 | 1 | 1 |
 * +--------------+---+---+------+---+---+---+---+
 * | user exec    | * | 1 | 1    | 1 | * | * | 1 |
 * +--------------+---+---+------+---+---+---+---+
 * | kernel read  | * | 1 | 0    | * | * | 1 | 1 |
 * +--------------+---+---+------+---+---+---+---+
 * | kernel write | 1 | 1 | 0    | * | 1 | 1 | 1 |
 * +--------------+---+---+------+---+---+---+---+
 * | kernel exec  | * | 1 | 0    | 1 | * | * | 1 |
 * +--------------+---+---+------+---+---+---+---+
 *
 * However, the dirty (D) and accessed (A) bits may be managed automaticaly
 * by hardware. In such case, setting of these bits is imperceptible from the
 * perspective of the software. To be compliant with the other ports,
 * we assume these bits to be unsupported and emulate them in software.
 *
 */

static const pt_entry_t pte_common = PTE_A | PTE_V;

static const pt_entry_t vm_prot_map[] = {
  [VM_PROT_READ] = PTE_SW_READ | PTE_R | pte_common,
  [VM_PROT_WRITE] = PTE_SW_WRITE | PTE_D | PTE_W | PTE_R | pte_common,
  [VM_PROT_READ | VM_PROT_WRITE] =
    PTE_SW_WRITE | PTE_SW_READ | PTE_D | PTE_W | PTE_R | pte_common,
  [VM_PROT_EXEC] = pte_common,
  [VM_PROT_READ | VM_PROT_EXEC] = PTE_SW_READ | PTE_X | PTE_R | pte_common,
  [VM_PROT_WRITE | VM_PROT_EXEC] =
    PTE_SW_WRITE | PTE_D | PTE_X | PTE_W | PTE_R | pte_common,
  [VM_PROT_READ | VM_PROT_WRITE | VM_PROT_EXEC] =
    PTE_SW_WRITE | PTE_SW_READ | PTE_D | PTE_X | PTE_W | PTE_R | pte_common,
};

/* Physical memory boundaries. */
static paddr_t dmap_paddr_base;
static paddr_t dmap_paddr_end;

static paddr_t kernel_pde;
static pmap_t kernel_pmap;

/* Bitmap of used ASIDs. */
static bitstr_t asid_used[bitstr_size(MAX_ASID)] = {0};
static SPIN_DEFINE(asid_lock, 0);

/*
 * This lock is used to protect the vm_page::pv_list field.
 * Order of acquiring locks:
 *  - pv_list_lock
 *  - pmap_t::mtx
 */
static MTX_DEFINE(pv_list_lock, 0);

/*
 * The list off all the user pmaps.
 * Order of acquiring locks:
 *  - `kernel_pmap->mtx`
 *  - user_pmaps_lock
 */
static MTX_DEFINE(user_pmaps_lock, 0);
static LIST_HEAD(, pmap) user_pmaps = LIST_HEAD_INITIALIZER(user_pmaps);

#define PAGE_OFFSET(va) ((va) & (PAGESIZE - 1))

/*
 * Helper functions.
 */

static inline bool is_valid_pde(pd_entry_t pde) {
  return pde & PTE_V;
}

static inline bool is_valid_pte(pt_entry_t pte) {
  return pte != 0;
}

static inline bool is_leaf_pte(pt_entry_t pte) {
  return pte & (PTE_X | PTE_W | PTE_R);
}

static bool user_addr_p(vaddr_t addr) {
  return addr >= USER_SPACE_BEGIN && addr < USER_SPACE_END;
}

static bool kern_addr_p(vaddr_t addr) {
  return addr >= KERNEL_SPACE_BEGIN && addr < KERNEL_SPACE_END;
}

inline vaddr_t pmap_start(pmap_t *pmap) {
  return pmap->asid ? USER_SPACE_BEGIN : KERNEL_SPACE_BEGIN;
}

inline vaddr_t pmap_end(pmap_t *pmap) {
  return pmap->asid ? USER_SPACE_END : KERNEL_SPACE_END;
}

inline bool pmap_address_p(pmap_t *pmap, vaddr_t va) {
  return va >= pmap_start(pmap) && va < pmap_end(pmap);
}

inline bool pmap_contains_p(pmap_t *pmap, vaddr_t start, vaddr_t end) {
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

static inline vaddr_t phys_to_dmap(paddr_t addr) {
  assert(addr >= dmap_paddr_base && addr < dmap_paddr_end);
  return (vaddr_t)(addr - dmap_paddr_base) + DMAP_VADDR_BASE;
}

static inline vaddr_t pg_dmap_addr(vm_page_t *pg) {
  return phys_to_dmap(pg->paddr);
}

static vm_page_t *pmap_pagealloc(void) {
  vm_page_t *pg = vm_page_alloc(1);
  pmap_zero_page(pg);
  return pg;
}

void pmap_zero_page(vm_page_t *pg) {
  bzero((void *)pg_dmap_addr(pg), PAGESIZE);
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
  assert(pg);

  TAILQ_INSERT_TAIL(&pmap->pte_pages, pg, pageq);

  klog("Page table for %p allocated at %p", va, pg->paddr);

  return pg->paddr;
}

static pt_entry_t *pmap_lookup_pte(pmap_t *pmap, vaddr_t va) {
  pd_entry_t *pdep;
  paddr_t pa = pmap->pde;

  /* Level 0 */
  pdep = (pd_entry_t *)phys_to_dmap(pa) + L0_INDEX(va);
  pd_entry_t pde = *pdep;
  if (!is_valid_pde(pde))
    return NULL;

  /* A direct map superpage? */
  if (is_leaf_pte(pde))
    return (pt_entry_t *)pdep;

  pa = PTE_TO_PA(pde);

  /* Level 1 */
  return (pt_entry_t *)phys_to_dmap(pa) + L1_INDEX(va);
}

static inline pd_entry_t make_pde(paddr_t pa) {
  return PA_TO_PTE(pa) | PTE_V;
}

static pt_entry_t make_pte(paddr_t pa, vm_prot_t prot, unsigned flags,
                           bool kernel) {
  pt_entry_t pte = PA_TO_PTE(pa) | vm_prot_map[prot];

  const pt_entry_t mask_on = (kernel) ? PTE_G : PTE_U;
  pte |= mask_on;

  const pt_entry_t mask_off = (kernel) ? 0 : PTE_D | PTE_A | PTE_W | PTE_V;
  pte &= ~mask_off;

  /* TODO(MichalBlk): set PMA attributes according to cache flags
   * passed in `flags`. */

  return pte;
}

static void pmap_write_pte(pmap_t *pmap, pt_entry_t *ptep, pt_entry_t pte,
                           vaddr_t va) {
  *ptep = pte;
  tlb_invalidate(va, pmap->asid);
}

/* Distribute new kernel L0 entry to all the user pmaps. */
static void pmap_distribute_l0(pmap_t *pmap, vaddr_t va, pd_entry_t pde) {
  if (pmap != pmap_kernel())
    return;

  SCOPED_MTX_LOCK(&user_pmaps_lock);

  pmap_t *user_pmap;
  LIST_FOREACH (user_pmap, &user_pmaps, pmap_link) {
    pd_entry_t *l0 = (pd_entry_t *)phys_to_dmap(user_pmap->pde);
    l0[L0_INDEX(va)] = pde;
  }
}

/* Return PTE pointer for `va`. Allocate page table if needed. */
static pt_entry_t *pmap_ensure_pte(pmap_t *pmap, vaddr_t va) {
  assert(mtx_owned(&pmap->mtx));

  pd_entry_t *pdep;
  paddr_t pa = pmap->pde;

  /* Level 0 */
  pdep = (pd_entry_t *)phys_to_dmap(pa) + L0_INDEX(va);
  if (!is_valid_pde(*pdep)) {
    pa = pmap_alloc_pde(pmap, va);
    *pdep = make_pde(pa);
    pmap_distribute_l0(pmap, va, *pdep);
  } else {
    pa = PTE_TO_PA(*pdep);
  }

  /* Level 1 */
  return (pt_entry_t *)phys_to_dmap(pa) + L1_INDEX(va);
}

/*
 * Wired memory interface.
 */

void pmap_kenter(vaddr_t va, paddr_t pa, vm_prot_t prot, unsigned flags) {
  pmap_t *pmap = pmap_kernel();

  assert(page_aligned_p(pa) && page_aligned_p(va));
  assert(pmap_address_p(pmap, va));

  klog("Enter unmanaged mapping from %p to %p", va, pa);

  pt_entry_t pte = make_pte(pa, prot, flags, true);

  WITH_MTX_LOCK (&pmap->mtx) {
    pt_entry_t *ptep = pmap_ensure_pte(pmap, va);
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
      pt_entry_t *ptep = pmap_lookup_pte(pmap, va + off);
      assert(ptep);
      pmap_write_pte(pmap, ptep, 0, va + off);
    }
  }
}

bool pmap_kextract(vaddr_t va, paddr_t *pap) {
  return pmap_extract(pmap_kernel(), va, pap);
}

/*
 * Pageable (user & kernel) memory interface.
 */

static void pmap_set_init_flags(vm_page_t *pg, bool kernel) {
  if (kernel)
    pg->flags |= PG_MODIFIED | PG_REFERENCED;
  else
    pg->flags &= ~(PG_MODIFIED | PG_REFERENCED);
}

void pmap_enter(pmap_t *pmap, vaddr_t va, vm_page_t *pg, vm_prot_t prot,
                unsigned flags) {
  paddr_t pa = pg->paddr;

  assert(page_aligned_p(va));
  assert(pmap_address_p(pmap, va));

  klog("Enter virtual mapping %p for frame %p", va, pa);

  bool kern_mapping = (pmap == pmap_kernel());
  pt_entry_t pte = make_pte(pa, prot, flags, kern_mapping);

  WITH_MTX_LOCK (&pv_list_lock) {
    WITH_MTX_LOCK (&pmap->mtx) {
      pv_entry_t *pv = pv_find(pmap, va, pg);
      if (!pv)
        pv_add(pmap, va, pg);
      pmap_set_init_flags(pg, kern_mapping);
      pt_entry_t *ptep = pmap_ensure_pte(pmap, va);
      pmap_write_pte(pmap, ptep, pte, va);
    }
  }
}

void pmap_remove(pmap_t *pmap, vaddr_t start, vaddr_t end) {
  assert(page_aligned_p(start) && page_aligned_p(end) && start < end);
  assert(pmap_contains_p(pmap, start, end));

  klog("Remove page mapping for address range %p - %p", start, end);

  WITH_MTX_LOCK (&pv_list_lock) {
    WITH_MTX_LOCK (&pmap->mtx) {
      for (vaddr_t va = start; va < end; va += PAGESIZE) {
        pt_entry_t *ptep = pmap_lookup_pte(pmap, va);
        if (!ptep || !is_valid_pte(*ptep))
          continue;
        paddr_t pa = PTE_TO_PA(*ptep);
        vm_page_t *pg = vm_page_find(pa);
        pv_remove(pmap, va, pg);
        pmap_write_pte(pmap, ptep, 0, va);
      }
    }
  }
}

static bool pmap_extract_nolock(pmap_t *pmap, vaddr_t va, paddr_t *pap) {
  if (!pmap_address_p(pmap, va))
    return false;

  pt_entry_t *ptep = pmap_lookup_pte(pmap, va);
  if (ptep == NULL || !is_valid_pte(*ptep))
    return false;

  paddr_t pa = PTE_TO_PA(*ptep);
  *pap = pa | PAGE_OFFSET(va);
  return true;
}

bool pmap_extract(pmap_t *pmap, vaddr_t va, paddr_t *pap) {
  SCOPED_MTX_LOCK(&pmap->mtx);
  return pmap_extract_nolock(pmap, va, pap);
}

void pmap_protect(pmap_t *pmap, vaddr_t start, vaddr_t end, vm_prot_t prot) {
  assert(page_aligned_p(start) && page_aligned_p(end) && start < end);
  assert(pmap_contains_p(pmap, start, end));

  klog("Change protection bits to %x for address range %p - %p", prot, start,
       end);

  bool kern_mapping = (pmap == pmap_kernel());
  const pt_entry_t mask_on = (kern_mapping) ? PTE_G : PTE_U;

  WITH_MTX_LOCK (&pmap->mtx) {
    for (vaddr_t va = start; va < end; va += PAGESIZE) {
      pt_entry_t *ptep = pmap_lookup_pte(pmap, va);
      if (!ptep || !is_valid_pte(*ptep))
        continue;
      pt_entry_t pte = (*ptep & ~PTE_PROT_MASK) | vm_prot_map[prot] | mask_on;
      pmap_write_pte(pmap, ptep, pte, va);
    }
  }
}

void pmap_page_remove(vm_page_t *pg) {
  SCOPED_MTX_LOCK(&pv_list_lock);

  while (!TAILQ_EMPTY(&pg->pv_list)) {
    pv_entry_t *pv = TAILQ_FIRST(&pg->pv_list);
    pmap_t *pmap = pv->pmap;
    vaddr_t va = pv->va;
    WITH_MTX_LOCK (&pmap->mtx) {
      TAILQ_REMOVE(&pg->pv_list, pv, page_link);
      TAILQ_REMOVE(&pmap->pv_list, pv, pmap_link);
      pt_entry_t *ptep = pmap_lookup_pte(pmap, va);
      assert(ptep);
      pmap_write_pte(pmap, ptep, 0, va);
    }
    pool_free(P_PV, pv);
  }
}

static void pmap_modify_flags(vm_page_t *pg, pt_entry_t set, pt_entry_t clr) {
  SCOPED_MTX_LOCK(&pv_list_lock);

  pv_entry_t *pv;
  TAILQ_FOREACH (pv, &pg->pv_list, page_link) {
    pmap_t *pmap = pv->pmap;
    vaddr_t va = pv->va;
    WITH_MTX_LOCK (&pmap->mtx) {
      pt_entry_t *ptep = pmap_lookup_pte(pmap, va);
      assert(ptep);
      pt_entry_t pte = *ptep;
      pte |= set;
      pte &= ~clr;
      pmap_write_pte(pmap, ptep, pte, va);
    }
  }
}

bool pmap_is_modified(vm_page_t *pg) {
  return pg->flags & PG_MODIFIED;
}

bool pmap_is_referenced(vm_page_t *pg) {
  return pg->flags & PG_REFERENCED;
}

void pmap_set_modified(vm_page_t *pg) {
  pg->flags |= PG_MODIFIED;
  pmap_modify_flags(pg, PTE_D | PTE_W, 0);
}

void pmap_set_referenced(vm_page_t *pg) {
  pg->flags |= PG_REFERENCED;
  pmap_modify_flags(pg, PTE_A | PTE_V, 0);
}

bool pmap_clear_modified(vm_page_t *pg) {
  bool prev = pmap_is_modified(pg);
  pg->flags &= ~PG_MODIFIED;
  pmap_modify_flags(pg, 0, PTE_D | PTE_W);
  return prev;
}

bool pmap_clear_referenced(vm_page_t *pg) {
  bool prev = pmap_is_referenced(pg);
  pg->flags &= ~PG_REFERENCED;
  pmap_modify_flags(pg, 0, PTE_A | PTE_V);
  return prev;
}

int pmap_emulate_bits(pmap_t *pmap, vaddr_t va, vm_prot_t prot) {
  paddr_t pa;

  WITH_MTX_LOCK (&pmap->mtx) {
    if (!pmap_extract_nolock(pmap, va, &pa))
      return EFAULT;

    pt_entry_t pte = *pmap_lookup_pte(pmap, va);

    if ((prot & VM_PROT_READ) && !(pte & PTE_SW_READ))
      return EACCES;

    if ((prot & VM_PROT_WRITE) && !(pte & PTE_SW_WRITE))
      return EACCES;

    if ((prot & VM_PROT_EXEC) && !(pte & PTE_X))
      return EACCES;
  }

  vm_page_t *pg = vm_page_find(pa);
  assert(pg);

  WITH_MTX_LOCK (&pv_list_lock) {
    /* Kernel non-pageable memory? */
    if (TAILQ_EMPTY(&pg->pv_list))
      return EINVAL;
  }

  pmap_set_referenced(pg);
  if (prot & VM_PROT_WRITE)
    pmap_set_modified(pg);

  return 0;
}

/*
 * Physical map management routines.
 */

static void pmap_setup(pmap_t *pmap) {
  assert(pmap->pde);
  pmap->asid = alloc_asid();
  pmap->satp = SATP_MODE_SV32 | ((paddr_t)pmap->asid << SATP_ASID_S) |
               (pmap->pde >> PAGE_SHIFT);
  mtx_init(&pmap->mtx, 0);
  TAILQ_INIT(&pmap->pte_pages);
  TAILQ_INIT(&pmap->pv_list);
}

void init_pmap(void) {
  kernel_pmap.pde = kernel_pde;
  pmap_setup(&kernel_pmap);
}

pmap_t *pmap_new(void) {
  pmap_t *pmap = pool_alloc(P_PMAP, M_ZERO);
  vm_page_t *pg = pmap_pagealloc();
  pmap->pde = pg->paddr;
  pmap_setup(pmap);

  /* Install kernel pagetables. */
  const size_t off = PAGESIZE / 2;
  WITH_MTX_LOCK (&kernel_pmap.mtx) {
    memcpy((void *)phys_to_dmap(pmap->pde) + off,
           (void *)phys_to_dmap(kernel_pde) + off, PAGESIZE / 2);
  }

  TAILQ_INSERT_TAIL(&pmap->pte_pages, pg, pageq);
  klog("Page directory table allocated at %p", pmap->pde);

  WITH_MTX_LOCK (&user_pmaps_lock) {
    LIST_INSERT_HEAD(&user_pmaps, pmap, pmap_link);
  }

  return pmap;
}

void pmap_activate(pmap_t *pmap) {
  SCOPED_NO_PREEMPTION();

  pmap_t *old = PCPU_GET(curpmap);
  if (pmap == old)
    return;

  csr_write(satp, pmap->satp);
  PCPU_SET(curpmap, pmap);

  __sfence_vma();
}

void pmap_delete(pmap_t *pmap) {
  assert(pmap != pmap_kernel());

  WITH_MTX_LOCK (&user_pmaps_lock) { LIST_REMOVE(pmap, pmap_link); }

  while (!TAILQ_EMPTY(&pmap->pv_list)) {
    pv_entry_t *pv = TAILQ_FIRST(&pmap->pv_list);
    vm_page_t *pg;
    paddr_t pa;
    assert(pmap_extract_nolock(pmap, pv->va, &pa));
    pg = vm_page_find(pa);
    WITH_MTX_LOCK (&pv_list_lock) { TAILQ_REMOVE(&pg->pv_list, pv, page_link); }
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

void pmap_bootstrap(paddr_t pd_pa, vaddr_t pd_va) {
  uint32_t dmap_size = kenv_get_ulong("mem_size");

  /* Obtain basic parameters. */
  dmap_paddr_base = kenv_get_ulong("mem_start");
  dmap_paddr_end = dmap_paddr_base + dmap_size;
  kernel_pde = pd_pa;

  /* Assume physical memory starts at the beginning of L0 region. */
  assert(is_aligned(dmap_paddr_base, L0_SIZE));

  /* We must have enough virtual addresses. */
  assert(dmap_size <= DMAP_MAX_SIZE);

  /* We assume 32-bit physical address space. */
  assert(dmap_paddr_base < dmap_paddr_end);

  klog("Physical memory range: %p - %p", dmap_paddr_base, dmap_paddr_end - 1);

  klog("dmap range: %p - %p", DMAP_VADDR_BASE, DMAP_VADDR_BASE + dmap_size - 1);

  /* Build direct map using 4MiB superpages. */
  pd_entry_t *pde = (void *)pd_va;
  size_t idx = L0_INDEX(DMAP_VADDR_BASE);
  for (paddr_t pa = dmap_paddr_base; pa < dmap_paddr_end; pa += L0_SIZE, idx++)
    pde[idx] = PA_TO_PTE(pa) | PTE_KERN;
}

/*
 * Increase usable kernel virtual address space to at least maxkvaddr.
 * Allocate page table (level 1) if needed.
 */
void pmap_growkernel(vaddr_t maxkvaddr) {
  assert(mtx_owned(&vm_kernel_end_lock));
  assert(maxkvaddr > vm_kernel_end);

  maxkvaddr = roundup2(maxkvaddr, L0_SIZE);
  assert(maxkvaddr <= DMAP_VADDR_BASE);
  pmap_t *pmap = pmap_kernel();

  WITH_MTX_LOCK (&pmap->mtx) {
    for (vaddr_t va = vm_kernel_end; va < maxkvaddr; va += L0_SIZE)
      (void)pmap_ensure_pte(pmap, va);
  }

  /*
   * kasan_grow calls pmap_kenter which acquires pmap->mtx.
   * But we are under vm_kernel_end_lock from kmem so it's safe to call
   * kasan_grow.
   */
  kasan_grow(maxkvaddr);

  vm_kernel_end = maxkvaddr;
}
