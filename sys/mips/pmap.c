#define KL_LOG KL_PMAP
#include <sys/klog.h>
#include <sys/mimiker.h>
#include <sys/libkern.h>
#include <sys/pool.h>
#include <mips/context.h>
#include <mips/mips.h>
#include <mips/tlb.h>
#include <mips/pmap.h>
#include <sys/kmem.h>
#include <sys/pcpu.h>
#include <sys/pmap.h>
#include <sys/vm_map.h>
#include <sys/thread.h>
#include <sys/signal.h>
#include <sys/spinlock.h>
#include <sys/mutex.h>
#include <sys/sched.h>
#include <sys/vm_physmem.h>
#include <bitstring.h>

static POOL_DEFINE(P_PMAP, "pmap", sizeof(pmap_t));

#define PDE_OF(pmap, vaddr) ((pmap)->pde[PDE_INDEX(vaddr)])
#define PTE_OF(pmap, vaddr) (PT_BASE[(vaddr) >> PTE_INDEX_SHIFT])
#define PTE_FRAME_ADDR(pte) (PTE_PFN_OF(pte) * PAGESIZE)
#define PAGE_OFFSET(x) ((x) & (PAGESIZE - 1))

static bool is_valid_pde(pde_t pde) {
  return pde & PDE_VALID;
}

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

static pmap_t kernel_pmap;
alignas(PAGESIZE) pte_t _kernel_pmap_pde[PT_ENTRIES];
static bitstr_t asid_used[bitstr_size(MAX_ASID)] = {0};
static spin_t *asid_lock = &SPIN_INITIALIZER(0);

static inline pte_t empty_pte(pmap_t *pmap) {
  return (pmap == pmap_kernel()) ? PTE_GLOBAL : 0;
}

static asid_t alloc_asid(void) {
  int free;
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

static pte_t pmap_pte_read(pmap_t *pmap, vaddr_t vaddr);

static void update_wired_pde(pmap_t *umap) {
  pmap_t *kmap = pmap_kernel();

  /* Both ENTRYLO0 and ENTRYLO1 must have G bit set for address translation
   * to skip ASID check. */
  tlbentry_t e = {.hi = PTE_VPN2(UPD_BASE),
                  .lo0 = PTE_GLOBAL,
                  .lo1 = PTE_PFN(MIPS_KSEG2_TO_PHYS(kmap->pde)) | PTE_KERNEL};

  if (umap) {
    pte_t pte = pmap_pte_read(kmap, (vaddr_t)umap->pde);
    e.lo0 = PTE_PFN(PTE_FRAME_ADDR(pte)) | PTE_KERNEL;
  }

  tlb_write(0, &e);
}

/* Page Table is accessible only through physical addresses. */
static void pmap_setup(pmap_t *pmap) {
  pmap->asid = alloc_asid();
  mtx_init(&pmap->mtx, 0);
  TAILQ_INIT(&pmap->pte_pages);
}

/* TODO: remove all mappings from TLB, evict related cache lines */
void pmap_reset(pmap_t *pmap) {
  while (!TAILQ_EMPTY(&pmap->pte_pages)) {
    vm_page_t *pg = TAILQ_FIRST(&pmap->pte_pages);
    TAILQ_REMOVE(&pmap->pte_pages, pg, pageq);
    vm_page_free(pg);
  }
  kmem_free(pmap->pde, PAGESIZE);
  free_asid(pmap->asid);
}

void init_pmap(void) {
  pmap_setup(&kernel_pmap);
  kernel_pmap.pde = _kernel_pmap_pde;
}

pmap_t *pmap_new(void) {
  pmap_t *pmap = pool_alloc(P_PMAP, M_ZERO);
  pmap_setup(pmap);

  pmap->pde = kmem_alloc(PAGESIZE, M_NOWAIT | M_ZERO);
  klog("Page directory table allocated at %p", (vaddr_t)pmap->pde);

  paddr_t pa;
  pmap_extract(pmap_kernel(), (vaddr_t)pmap->pde, &pa);

  pmap->pde[PDE_INDEX(PT_BASE)] = PTE_PFN(pa) | PTE_KERNEL;

  return pmap;
}

void pmap_delete(pmap_t *pmap) {
  pmap_reset(pmap);
  pool_free(P_PMAP, pmap);
}

/* pmap_pte_write calls pmap_add_pde so we need this declaration */
static pde_t pmap_add_pde(pmap_t *pmap, vaddr_t vaddr);

/*! \brief Reads the PTE mapping virtual address \a vaddr. */
static pte_t pmap_pte_read(pmap_t *pmap, vaddr_t vaddr) {
  pde_t pde = PDE_OF(pmap, vaddr);
  if (!is_valid_pde(pde))
    return 0;
  return PTE_OF(pmap, vaddr);
}

/*! \brief Writes \a pte as the new PTE mapping virtual address \a vaddr. */
static void pmap_pte_write(pmap_t *pmap, vaddr_t vaddr, pte_t pte,
                           unsigned flags) {
  unsigned cacheflags = flags & PMAP_CACHE_MASK;

  if (cacheflags == PMAP_NOCACHE)
    pte |= PTE_CACHE_UNCACHED;
  else if (cacheflags == PMAP_WRITE_THROUGH)
    pte |= PTE_CACHE_WRITE_THROUGH;
  else
    pte |= PTE_CACHE_WRITE_BACK;

  pde_t pde = PDE_OF(pmap, vaddr);
  if (!is_valid_pde(pde))
    pde = pmap_add_pde(pmap, vaddr);
  PTE_OF(pmap, vaddr) = pte;
  tlb_invalidate(PTE_VPN2(vaddr) | PTE_ASID(pmap->asid));
}

/* Add PT to PD so kernel can handle access to @vaddr. */
static pde_t pmap_add_pde(pmap_t *pmap, vaddr_t vaddr) {
  assert(!is_valid_pde(PDE_OF(pmap, vaddr)));

  vm_page_t *pg = vm_page_alloc(1);
  pde_t pde = PTE_PFN(pg->paddr) | PTE_KERNEL;
  PDE_OF(pmap, vaddr) = pde;

  pte_t *pte = &PTE_OF(pmap, vaddr & PDE_INDEX_MASK);
  tlb_invalidate(PTE_VPN2((vaddr_t)pte) | PTE_ASID(pmap->asid));

  TAILQ_INSERT_TAIL(&pmap->pte_pages, pg, pageq);
  klog("Page table for %08lx allocated at %08lx", vaddr & PDE_INDEX_MASK, pte);

  /* Must initialize to PTE_GLOBAL, look at comment in update_wired_pde! */
  for (int i = 0; i < PT_ENTRIES; i++)
    pte[i] = PTE_GLOBAL;

  return pde;
}

/* TODO: implement */
void pmap_remove_pde(pmap_t *pmap, vaddr_t vaddr);

static pte_t vm_prot_map[] = {
  [VM_PROT_NONE] = 0,
  [VM_PROT_READ] = PTE_VALID | PTE_NO_EXEC,
  [VM_PROT_WRITE] = PTE_VALID | PTE_DIRTY | PTE_NO_READ | PTE_NO_EXEC,
  [VM_PROT_READ | VM_PROT_WRITE] = PTE_VALID | PTE_DIRTY | PTE_NO_EXEC,
  [VM_PROT_EXEC] = PTE_VALID | PTE_NO_READ,
  [VM_PROT_READ | VM_PROT_EXEC] = PTE_VALID,
  [VM_PROT_WRITE | VM_PROT_EXEC] = PTE_VALID | PTE_DIRTY | PTE_NO_READ,
  [VM_PROT_READ | VM_PROT_WRITE | VM_PROT_EXEC] = PTE_VALID | PTE_DIRTY,
};

void pmap_kenter(vaddr_t va, paddr_t pa, vm_prot_t prot, unsigned flags) {
  pmap_t *pmap = pmap_kernel();

  assert(page_aligned_p(pa) && page_aligned_p(va));
  assert(pmap_address_p(pmap, va));
  assert(pa != 0);

  klog("Enter unmanaged mapping from %p to %p", va, pa);

  pte_t pte = vm_prot_map[prot] | PTE_GLOBAL;

  WITH_MTX_LOCK (&pmap->mtx)
    pmap_pte_write(pmap, va, PTE_PFN(pa) | pte, flags);
}

void pmap_enter(pmap_t *pmap, vaddr_t va, vm_page_t *pg, vm_prot_t prot,
                unsigned flags) {
  vaddr_t va_end = va + PG_SIZE(pg);
  paddr_t pa = PG_START(pg);

  assert(page_aligned_p(va));
  assert(pmap_contains_p(pmap, va, va_end));
  assert(pa != 0);

  klog("Enter virtual mapping %p-%p for frame %p", va, va_end, pa);

  /* Mark user pages as non-referenced & non-modified. */
  pte_t mask = (pmap == pmap_kernel()) ? 0 : PTE_VALID | PTE_DIRTY;
  pte_t pte = (vm_prot_map[prot] & ~mask) | empty_pte(pmap);

  WITH_MTX_LOCK (&pmap->mtx) {
    for (; va < va_end; va += PAGESIZE, pa += PAGESIZE)
      pmap_pte_write(pmap, va, PTE_PFN(pa) | pte, flags);
  }
}

void pmap_kremove(vaddr_t start, vaddr_t end) {
  pmap_t *pmap = pmap_kernel();

  assert(page_aligned_p(start) && page_aligned_p(end) && start < end);
  assert(pmap_contains_p(pmap, start, end));

  klog("%s: remove unmanaged mapping for %p - %p range", __func__, start,
       end - 1);

  WITH_MTX_LOCK (&kernel_pmap.mtx) {
    for (vaddr_t va = start; va < end; va += PAGESIZE)
      pmap_pte_write(pmap, va, PTE_GLOBAL, 0);
  }
}

void pmap_remove(pmap_t *pmap, vaddr_t start, vaddr_t end) {
  assert(page_aligned_p(start) && page_aligned_p(end) && start < end);
  assert(pmap_contains_p(pmap, start, end));

  klog("Remove page mapping for address range %p-%p", start, end);

  WITH_MTX_LOCK (&pmap->mtx) {
    for (vaddr_t va = start; va < end; va += PAGESIZE)
      pmap_pte_write(pmap, va, empty_pte(pmap), 0);

    /* TODO: Deallocate empty page table fragment by calling pmap_remove_pde. */
  }
}

void pmap_protect(pmap_t *pmap, vaddr_t start, vaddr_t end, vm_prot_t prot) {
  assert(page_aligned_p(start) && page_aligned_p(end) && start < end);
  assert(pmap_contains_p(pmap, start, end));

  klog("Change protection bits to %x for address range %p-%p", prot, start,
       end);

  WITH_MTX_LOCK (&pmap->mtx) {
    for (vaddr_t va = start; va < end; va += PAGESIZE) {
      pte_t pte = pmap_pte_read(pmap, va);
      if (pte == 0)
        continue;
      pmap_pte_write(pmap, va, (pte & ~PTE_PROT_MASK) | vm_prot_map[prot], 0);
    }
  }
}

bool pmap_extract(pmap_t *pmap, vaddr_t va, paddr_t *pap) {
  if (!pmap_address_p(pmap, va))
    return false;

  pte_t pte = pmap_pte_read(pmap, va);
  if (pte == empty_pte(pmap))
    return false;

  *pap = PTE_FRAME_ADDR(pte) | PAGE_OFFSET(va);
  return true;
}

#define PG_KSEG0_ADDR(pg) (void *)(MIPS_PHYS_TO_KSEG0((pg)->paddr))

void pmap_zero_page(vm_page_t *pg) {
  bzero(PG_KSEG0_ADDR(pg), PAGESIZE);
}

void pmap_copy_page(vm_page_t *src, vm_page_t *dst) {
  memcpy(PG_KSEG0_ADDR(dst), PG_KSEG0_ADDR(src), PAGESIZE);
}

#undef PG_KSEG0_ADDR

/* TODO: at any given moment there're two page tables in use:
 *  - kernel-space pmap for kseg2 & kseg3
 *  - user-space pmap for useg
 * ks_pmap is shared across all vm_maps. How to switch us_pmap efficiently?
 * Correct solution needs to make sure no unwanted entries are left in TLB and
 * caches after the switch.
 */

void pmap_activate(pmap_t *pmap) {
  SCOPED_NO_PREEMPTION();

  PCPU_SET(curpmap, pmap);
  update_wired_pde(pmap);

  /* Set ASID for current process */
  mips32_setentryhi(pmap ? pmap->asid : 0);
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

void tlb_exception_handler(ctx_t *ctx) {
  thread_t *td = thread_self();

  int code = (_REG(ctx, CAUSE) & CR_X_MASK) >> CR_X_SHIFT;
  vaddr_t vaddr = _REG(ctx, BADVADDR);

  klog("%s at $%08x, caused by reference to $%08lx!", exceptions[code],
       _REG(ctx, EPC), vaddr);

  pmap_t *pmap = pmap_lookup(vaddr);
  if (!pmap) {
    klog("No physical map defined for %08lx address!", vaddr);
    goto fault;
  }

  pte_t pte = pmap_pte_read(pmap, vaddr);
  paddr_t pa = PTE_FRAME_ADDR(pte);

  if (pa) {
    vm_page_t *pg = vm_page_find(pa);

    if ((pte & PTE_VALID) == 0 && code == EXC_TLBL) {
      pmap_set_referenced(pg);
      pmap_pte_write(pmap, vaddr, pte | PTE_VALID, 0);
      return;
    }
    if ((pte & PTE_DIRTY) == 0 && (code == EXC_TLBS || code == EXC_MOD)) {
      pmap_set_referenced(pg);
      pmap_set_modified(pg);
      pmap_pte_write(pmap, vaddr, pte | PTE_DIRTY | PTE_VALID, 0);
      return;
    }
  }

  vm_map_t *vmap = vm_map_lookup(vaddr);
  if (!vmap) {
    klog("No virtual address space defined for %08lx!", vaddr);
    goto fault;
  }
  vm_prot_t access = (code == EXC_TLBL) ? VM_PROT_READ : VM_PROT_WRITE;
  int ret = vm_page_fault(vmap, vaddr, access);
  if (ret == 0)
    return;

fault:
  if (td->td_onfault) {
    /* handle copyin / copyout faults */
    _REG(ctx, EPC) = td->td_onfault;
    td->td_onfault = 0;
  } else if (td->td_proc) {
    /* Panic when process running in kernel space uses wrong pointer. */
    if (kern_mode_p(ctx))
      kernel_oops(ctx);

    /* Send a segmentation fault signal to the user program. */
    sig_trap(ctx, SIGSEGV);
  } else {
    /* Panic when kernel-mode thread uses wrong pointer. */
    kernel_oops(ctx);
  }
}
