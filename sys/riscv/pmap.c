#define KL_LOG KL_PMAP
#include <bitstring.h>
#include <sys/kasan.h>
#include <sys/kenv.h>
#include <sys/klog.h>
#include <sys/libkern.h>
#include <sys/mimiker.h>
#include <sys/mutex.h>
#include <sys/pcpu.h>
#include <sys/pool.h>
#include <sys/pmap.h>
#include <sys/spinlock.h>
#include <sys/vm_physmem.h>
#include <riscv/pmap.h>
#include <riscv/riscvreg.h>
#include <riscv/tlb.h>
#include <riscv/vm_param.h>

typedef struct pmap {
  mtx_t mtx;               /* protects all fields in this structure */
  asid_t asid;             /* address space identifier */
  paddr_t pde;             /* directory page table physical address */
  vm_pagelist_t pte_pages; /* pages we allocate in page table */
} pmap_t;

/* Physical memory boundaries. */
static paddr_t dmap_paddr_base;
static paddr_t dmap_paddr_end;

static paddr_t kernel_pde;
static pmap_t kernel_pmap;

/* Bitmap of used ASIDs. */
static bitstr_t asid_used[bitstr_size(MAX_ASID)] = {0};
static SPIN_DEFINE(asid_lock, 0);

static POOL_DEFINE(P_PMAP, "pmap", sizeof(pmap_t));

/*
 * The following table describes which access bits need to be set in page table
 * entry for successful memory translation by MMU. Other configurations cause
 * memory fault - see riscv/trap.c.
 *
 * +--------------+---+---+------+---+---+---+
 * |    access    | D | A | USER | X | W | R |
 * +==============+===+===+======+===+===+===+
 * | user read    | * | 1 | 1    | * | * | 1 |
 * +--------------+---+---+------+---+---+---+
 * | user write   | 1 | 1 | 1    | * | 1 | 1 |
 * +--------------+---+---+------+---+---+---+
 * | user exec    | * | 1 | 1    | 1 | * | * |
 * +--------------+---+---+------+---+---+---+
 * | kernel read  | * | 1 | 0    | * | * | 1 |
 * +--------------+---+---+------+---+---+---+
 * | kernel write | 1 | 1 | 0    | * | 1 | 1 |
 * +--------------+---+---+------+---+---+---+
 * | kernel exec  | * | 1 | 0    | 1 | * | * |
 * +--------------+---+---+------+---+---+---+
 */

static const pt_entry_t pte_common = PTE_A | PTE_V;

static const pt_entry_t vm_prot_map[] = {
  [VM_PROT_READ] = PTE_R | pte_common,
  [VM_PROT_WRITE] = PTE_D | PTE_W | PTE_R | pte_common,
  [VM_PROT_READ | VM_PROT_WRITE] = PTE_D | PTE_W | PTE_R | pte_common,
  [VM_PROT_EXEC] = PTE_V | pte_common,
  [VM_PROT_READ | VM_PROT_EXEC] = PTE_X | PTE_R | pte_common,
  [VM_PROT_WRITE | VM_PROT_EXEC] = PTE_D | PTE_X | PTE_W | PTE_R | pte_common,
  [VM_PROT_READ | VM_PROT_WRITE | VM_PROT_EXEC] =
    PTE_D | PTE_X | PTE_W | PTE_R | pte_common,
};

#define PG_DMAP_ADDR(pg) ((void *)phys_to_dmap((pg)->paddr))
#define PAGE_OFFSET(va) ((va) & (PAGESIZE - 1))

/*
 * Helper functions.
 */

static inline bool is_valid_pde(pd_entry_t pde) {
  return pde & PTE_V;
}

static inline bool is_valid_pte(pt_entry_t pte) {
  return pte & PTE_V;
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

static vm_page_t *pmap_pagealloc(void) {
  vm_page_t *pg = vm_page_alloc(1);
  pmap_zero_page(pg);
  return pg;
}

void pmap_zero_page(vm_page_t *pg) {
  bzero(PG_DMAP_ADDR(pg), PAGESIZE);
}

void pmap_copy_page(vm_page_t *src, vm_page_t *dst) {
  memcpy(PG_DMAP_ADDR(dst), PG_DMAP_ADDR(src), PAGESIZE);
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

static __unused void free_asid(asid_t asid) {
  klog("free_asid(%d)", asid);
  SCOPED_SPIN_LOCK(&asid_lock);
  bit_clear(asid_used, (unsigned)asid);
  tlb_invalidate_asid(asid);
}

/*
 * Routines for accessing page table entries.
 */

static paddr_t pmap_alloc_pde(pmap_t *pmap, vaddr_t va) {
  vm_page_t *pg = pmap_pagealloc();

  TAILQ_INSERT_TAIL(&pmap->pte_pages, pg, pageq);

  klog("Page table for %p allocated at %p", va, pg->paddr);

  return pg->paddr;
}

static pt_entry_t *pmap_lookup_pte(pmap_t *pmap, vaddr_t va) {
  pd_entry_t *pdep;
  paddr_t pa = pmap->pde;

  /* Level 0 */
  pdep = (pd_entry_t *)phys_to_dmap(pa) + L0_INDEX(va);
  if (!is_valid_pde(*pdep))
    return NULL;
  pa = PTE_TO_PA(*pdep);

  /* Level 1 */
  return (pt_entry_t *)phys_to_dmap(pa) + L1_INDEX(va);
}

static pt_entry_t make_pte(paddr_t pa, vm_prot_t prot, unsigned flags,
                           bool kernel) {
  pt_entry_t pte = PA_TO_PTE(pa) | vm_prot_map[prot];

  const pt_entry_t mask_on = (kernel) ? PTE_G : PTE_U;
  pte |= mask_on;

  const pt_entry_t mask_off = (kernel) ? 0 : PTE_D | PTE_A;
  pte &= ~mask_off;

  /* TODO(MichalBlk): set PMA attributes according to cache flags. */

  return pte;
}

static void pmap_write_pte(pmap_t *pmap, pt_entry_t *ptep, pt_entry_t pte,
                           vaddr_t va) {
  *ptep = pte;
  tlb_invalidate(va, pmap->asid);
}

/* Return PTE pointer for `va`. Allocate page table if needed. */
static pt_entry_t *pmap_ensure_pte(pmap_t *pmap, vaddr_t va) {
  pd_entry_t *pdep;
  paddr_t pa = pmap->pde;

  /* Level 0 */
  pdep = (pd_entry_t *)phys_to_dmap(pa) + L0_INDEX(va);
  if (!is_valid_pde(*pdep)) {
    *pdep = PA_TO_PTE(pmap_alloc_pde(pmap, va)) | PTE_V;
  }
  pa = PTE_TO_PA(*pdep);

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

bool pmap_is_modified(vm_page_t *pg) {
  panic("Not implemented!");
}

bool pmap_is_referenced(vm_page_t *pg) {
  panic("Not implemented!");
}

void pmap_set_modified(vm_page_t *pg) {
  panic("Not implemented!");
}

void pmap_set_referenced(vm_page_t *pg) {
  panic("Not implemented!");
}

bool pmap_clear_modified(vm_page_t *pg) {
  panic("Not implemented!");
}

bool pmap_clear_referenced(vm_page_t *pg) {
  panic("Not implemented!");
}

void pmap_enter(pmap_t *pmap, vaddr_t va, vm_page_t *pg, vm_prot_t prot,
                unsigned flags) {
  panic("Not implemented!");
}

void pmap_remove(pmap_t *pmap, vaddr_t start, vaddr_t end) {
  panic("Not implemented!");
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
  panic("Not implemented!");
}

void pmap_page_remove(vm_page_t *pg) {
  panic("Not implemented!");
}

int pmap_emulate_bits(pmap_t *pmap, vaddr_t va, vm_prot_t prot) {
  panic("Not implemented!");
}

/*
 * Physical map management routines.
 */

static void pmap_setup(pmap_t *pmap) {
  pmap->asid = alloc_asid();
  mtx_init(&pmap->mtx, 0);
  TAILQ_INIT(&pmap->pte_pages);
}

void init_pmap(void) {
  pmap_setup(&kernel_pmap);
  kernel_pmap.pde = kernel_pde;
}

pmap_t *pmap_new(void) {
  pmap_t *pmap = pool_alloc(P_PMAP, M_ZERO);
  pmap_setup(pmap);

  vm_page_t *pg = pmap_pagealloc();
  TAILQ_INSERT_TAIL(&pmap->pte_pages, pg, pageq);
  pmap->pde = pg->paddr;
  klog("Page directory table allocated at %p", pmap->pde);

  return pmap;
}

void pmap_activate(pmap_t *pmap) {
  panic("Not implemented!");
}

void pmap_delete(pmap_t *pmap) {
  panic("Not implemented!");
}

void pmap_bootstrap(paddr_t pd_pa, pd_entry_t *pd_va) {
  uint32_t dmap_size = kenv_get_ulong("mem_size");

  /* Obtain basic parameters. */
  dmap_paddr_base = kenv_get_ulong("mem_start");
  dmap_paddr_end = dmap_paddr_base + dmap_size;
  kernel_pde = pd_pa;

  /* Assume the physical memory starts at the beginning of an L0 region. */
  assert(is_aligned(dmap_paddr_base, L0_SIZE));

  /* We must have enough virtual addresses. */
  assert(dmap_size <= DMAP_MAX_SIZE);

  /* We assume maximum physical address < 2^((sizeof(paddr_t) * 8)). */
  assert(dmap_paddr_base < dmap_paddr_end);

  klog("Physical memory range: %p - %p", dmap_paddr_base, dmap_paddr_end - 1);

  /* Build a direct map using 4MiB superpages. */
  size_t idx = L0_INDEX(DMAP_VADDR_BASE);
  for (paddr_t pa = dmap_paddr_base; pa < dmap_paddr_end; pa += L0_SIZE, idx++)
    pd_va[idx] = PA_TO_PTE(pa) | PTE_KERN;
}

/* Increase usable kernel virtual address space to at least maxkvaddr.
 * Allocate page table (level 1) if needed. */
void pmap_growkernel(vaddr_t maxkvaddr) {
  assert(mtx_owned(&vm_kernel_end_lock));
  assert(maxkvaddr > vm_kernel_end);

  maxkvaddr = roundup2(maxkvaddr, L0_SIZE);

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
