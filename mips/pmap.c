#define KL_LOG KL_PMAP
#include <klog.h>
#include <stdc.h>
#include <malloc.h>
#include <mips/exc.h>
#include <mips/mips.h>
#include <mips/tlb.h>
#include <pcpu.h>
#include <pmap.h>
#include <sync.h>
#include <vm_map.h>
#include <thread.h>
#include <ktest.h>
#include <signal.h>
#include <sysinit.h>

static MALLOC_DEFINE(M_PMAP, "pmap", 1, 1);

#define PTE_MASK 0xfffff000
#define PTE_SHIFT 12
#define PDE_MASK 0xffc00000
#define PDE_SHIFT 22

#define PTE_INDEX(addr) (((addr)&PTE_MASK) >> PTE_SHIFT)
#define PDE_INDEX(addr) (((addr)&PDE_MASK) >> PDE_SHIFT)

#define PTE_OF(pmap, addr) ((pmap)->pte[PTE_INDEX(addr)])
#define PDE_OF(pmap, addr) ((pmap)->pde[PDE_INDEX(addr)])
#define PTF_ADDR_OF(vaddr) (PT_BASE + PDE_INDEX(vaddr) * PTF_SIZE)
#define PTE_ADDR_OF(vaddr) (PT_BASE + PTE_INDEX(vaddr) * sizeof(pte_t))

#define PD_ENTRIES 1024 /* page directory entries */
#define PD_SIZE (PD_ENTRIES * sizeof(pde_t))
#define PTF_ENTRIES 1024 /* page table fragment entries */
#define PTF_SIZE (PTF_ENTRIES * sizeof(pte_t))
#define PT_ENTRIES (PD_ENTRIES * PTF_ENTRIES)
#define PT_SIZE (PT_ENTRIES * sizeof(pte_t))

#define KERNEL_PD_BASE (PT_BASE + PT_SIZE)
#define USER_PD_BASE (KERNEL_PD_BASE + PD_SIZE)

#define PMAP_KERNEL_BEGIN MIPS_KSEG2_START
#define PMAP_KERNEL_END 0xfffff000 /* kseg2 & kseg3 */
#define PMAP_USER_BEGIN 0x00000000
#define PMAP_USER_END MIPS_KSEG0_START /* useg */

static const vm_addr_t PT_HOLE_START = PTE_ADDR_OF(MIPS_KSEG0_START);
static const vm_addr_t PT_HOLE_END = PTE_ADDR_OF(MIPS_KSEG2_START);

static bool pde_is_valid(pde_t pde) {
  return (pde).lo & PTE_VALID;
}

static bool in_user_space(vm_addr_t addr) {
  return (addr >= PMAP_USER_BEGIN && addr < PMAP_USER_END);
}

static bool in_kernel_space(vm_addr_t addr) {
  return (addr >= PMAP_KERNEL_BEGIN && addr < PMAP_KERNEL_END);
}

static tlblo_t global_of(vm_addr_t vaddr) {
  return in_kernel_space(vaddr) ? PTE_GLOBAL : 0;
}

static bool is_referenced(pte_t *pte) {
  return (pte->ext_flags & PTE_EXT_REFERENCED);
}

static bool is_modified(pte_t *pte) {
  return (pte->ext_flags * PTE_EXT_MODIFIED);
}

static pmap_t kernel_pmap;
static asid_t asid_counter;

static asid_t get_new_asid() {
  if (asid_counter < MAX_ASID)
    return asid_counter++; /* TODO this needs to be atomic increment */
  else
    panic("Out of asids!");
}

static void pmap_setup_pd_wired_tlb_entry(pmap_t *pmap) {
  bool kernel_setup = (pmap == &kernel_pmap);

  tlbhi_t hi = (tlbhi_t)pmap->pde | pmap->asid;
  tlblo_t flags = PTE_VALID | PTE_DIRTY | (kernel_setup ? PTE_GLOBAL : 0);
  tlblo_t lo0 = PTE_PFN(pmap->pde_page->paddr) | flags;
  tlblo_t lo1 = lo0 | PTE_PFN(PAGESIZE);

  unsigned index = kernel_setup ? TLB_KERNEL_PDE_ENTRY : TLB_USER_PDE_ENTRY;

  tlb_write_index(hi, lo0, lo1, index);
}

static void pmap_setup(pmap_t *pmap, vm_addr_t start, vm_addr_t end) {
  bool kernel_setup = (pmap == &kernel_pmap);

  pmap->pte = (pte_t *)PT_BASE;
  pmap->pde_page = pm_alloc(2);
  pmap->pde = kernel_setup ? (pde_t *)KERNEL_PD_BASE : (pde_t *)USER_PD_BASE;
  pmap->start = start;
  pmap->end = end;
  pmap->asid = get_new_asid();
  klog("Page directory table allocated at %08lx", (vm_addr_t)pmap->pde);
  TAILQ_INIT(&pmap->pte_pages);

  /* Initialize PD using addresses from KSEG0 (to bypass TLB translation) */
  pde_t *pde = (pde_t *)pmap->pde_page->vaddr;

  for (int i = 0; i < PD_ENTRIES; i++) {
    pde[i].lo = global_of(i * PTF_ENTRIES * PAGESIZE);
    pde[i].unused = 0;
  }
}

/* TODO: remove all mappings from TLB, evict related cache lines */
/* Thanks to ASIDs we don't have to remove those entries,
 * Just make sure that the pmap is no longer the active one */
void pmap_reset(pmap_t *pmap) {
  CRITICAL_SECTION {
    if (PCPU_GET(curpmap) == pmap) {
      mips32_set_c0(C0_ENTRYHI, 0);
      PCPU_SET(curpmap, 0);
    }
  }

  while (!TAILQ_EMPTY(&pmap->pte_pages)) {
    vm_page_t *pg = TAILQ_FIRST(&pmap->pte_pages);
    TAILQ_REMOVE(&pmap->pte_pages, pg, pt.list);
    pm_free(pg);
  }
  pm_free(pmap->pde_page);
  memset(pmap, 0, sizeof(pmap_t)); /* Set up for reuse. */
}

void pmap_init() {
  vm_addr_t setup_from = (vm_addr_t)(PMAP_KERNEL_BEGIN + PT_SIZE + 2 * PD_SIZE);
  vm_addr_t setup_to = (vm_addr_t)PMAP_KERNEL_END;

  pmap_setup(&kernel_pmap, setup_from, setup_to);
  pmap_setup_pd_wired_tlb_entry(&kernel_pmap);
}

pmap_t *pmap_new() {
  pmap_t *pmap = kmalloc(M_PMAP, sizeof(pmap_t), M_ZERO);
  pmap_setup(pmap, PMAP_USER_BEGIN, PMAP_USER_END);
  return pmap;
}

void pmap_delete(pmap_t *pmap) {
  pmap_reset(pmap);
  kfree(M_PMAP, pmap);
}

/* Add PT to PD so kernel can handle access to @vaddr. */
static void pmap_add_pde(pmap_t *pmap, vm_addr_t vaddr) {
  /* assume page table fragment not present in physical memory */
  assert(!pde_is_valid(PDE_OF(pmap, vaddr)));

  vm_page_t *pg = pm_alloc(2);
  TAILQ_INSERT_TAIL(&pmap->pte_pages, pg, pt.list);
  klog("Page table fragment %08lx allocated at %08lx", PTF_ADDR_OF(vaddr),
       pg->paddr);

  pde_t *pde = &PDE_OF(pmap, vaddr);

  vm_addr_t ptf_start = PTF_ADDR_OF(vaddr);
  tlblo_t flags = PTE_VALID | PTE_DIRTY | global_of(vaddr);
  tlblo_t lo0 = PTE_PFN(pg->paddr) | flags;
  tlblo_t lo1 = PTE_PFN(pg->paddr + PAGESIZE) | flags;
  pde->lo = lo0;
  pde->unused = 0;

  tlb_overwrite_random(ptf_start | pmap->asid, lo0, lo1);

  pte_t *pte = (pte_t *)PTF_ADDR_OF(vaddr);
  for (int i = 0; i < PTF_ENTRIES; i++) {
    pte[i].lo = PTE_GLOBAL;
    pte[i].ext_flags = 0;
  }
}

/* Check if PDE for given vaddr resides already in TLB */
static void pmap_assure_pde_in_tlb(pmap_t *pmap, vm_addr_t vaddr) {
  assert(pde_is_valid(PDE_OF(pmap, vaddr)));

  pde_t *pde = &PDE_OF(pmap, vaddr);
  vm_addr_t ptf_start = PTF_ADDR_OF(vaddr);
  tlbhi_t hi = ptf_start | pmap->asid;
  tlblo_t lo0, lo1;

  if (tlb_probe(hi, &lo0, &lo1) >= 0)
    return;

  lo0 = pde->lo;
  lo1 = pde->lo | PTE_PFN(PAGESIZE);
  tlb_overwrite_random(hi, lo0, lo1);
}

/* TODO: implement */
void pmap_remove_pde(pmap_t *pmap, vm_addr_t vaddr);

#if 0
/* Used if CPU implements RI and XI bits in ENTRYLO. */
static tlblo_t vm_prot_map[] = {
  [VM_PROT_NONE] = 0,
  [VM_PROT_READ] = PTE_VALID|PTE_NO_EXEC,
  [VM_PROT_WRITE] = PTE_VALID|PTE_DIRTY|PTE_NO_READ|PTE_NO_EXEC,
  [VM_PROT_READ|VM_PROT_WRITE] = PTE_VALID|PTE_DIRTY|PTE_NO_EXEC,
  [VM_PROT_EXEC] = PTE_VALID|PTE_NO_READ,
  [VM_PROT_READ|VM_PROT_EXEC] = PTE_VALID,
  [VM_PROT_WRITE|VM_PROT_EXEC] = PTE_VALID|PTE_DIRTY|PTE_NO_READ,
  [VM_PROT_READ|VM_PROT_WRITE|VM_PROT_EXEC] = PTE_VALID|PTE_DIRTY,
};
#else
static tlblo_t vm_prot_map[] = {
    [VM_PROT_NONE] = 0,
    [VM_PROT_READ] = PTE_VALID,
    [VM_PROT_WRITE] = PTE_VALID | PTE_DIRTY,
    [VM_PROT_READ | VM_PROT_WRITE] = PTE_VALID | PTE_DIRTY,
    [VM_PROT_EXEC] = PTE_VALID,
    [VM_PROT_READ | VM_PROT_EXEC] = PTE_VALID,
    [VM_PROT_WRITE | VM_PROT_EXEC] = PTE_VALID | PTE_DIRTY,
    [VM_PROT_READ | VM_PROT_WRITE | VM_PROT_EXEC] = PTE_VALID | PTE_DIRTY,
};
#endif

/* TODO: what about caches? */
static void pmap_set_pte(pmap_t *pmap, vm_addr_t vaddr, pm_addr_t paddr,
                         vm_prot_t prot) {
  if (!pde_is_valid(PDE_OF(pmap, vaddr)))
    pmap_add_pde(pmap, vaddr);
  else
    pmap_assure_pde_in_tlb(pmap, vaddr);

  pte_t *pte = &PTE_OF(pmap, vaddr);
  pte->lo = PTE_PFN(paddr) | global_of(vaddr);
  pte->ext_flags = vm_prot_map[prot];

  klog("Add mapping for page %08lx (PTE at %08lx)", (vaddr & PTE_MASK),
       (vm_addr_t)pte);

  /* invalidate corresponding entry in tlb */
  tlb_invalidate(PTE_VPN2(vaddr) | PTE_ASID(pmap->asid));
}

/* TODO: what about caches? */
static void pmap_clear_pte(pmap_t *pmap, vm_addr_t vaddr) {
  pte_t *pte = &PTE_OF(pmap, vaddr);
  pte->lo = 0;
  pte->ext_flags = 0;

  klog("Remove mapping for page %08lx (PTE at %08lx)", (vaddr & PTE_MASK),
       (vm_addr_t)pte);

  /* invalidate corresponding entry in tlb */
  tlb_invalidate(PTE_VPN2(vaddr) | PTE_ASID(pmap->asid));

  /* TODO: Deallocate empty page table fragment by calling pmap_remove_pde. */
}

/* TODO: what about caches? */
static void pmap_change_pte(pmap_t *pmap, vm_addr_t vaddr, vm_prot_t prot) {
  pmap_assure_pde_in_tlb(pmap, vaddr);
  pte_t *pte = &PTE_OF(pmap, vaddr);

  tlblo_t prot_mapped = vm_prot_map[prot];

  pte->lo &= ~PTE_PROT_MASK;
  pte->ext_flags = (pte->ext_flags & ~PTE_PROT_MASK) | prot_mapped;

  if (is_referenced(pte) && (pte->ext_flags & PTE_VALID))
    pte->lo |= PTE_VALID;
  if (is_modified(pte) && (pte->ext_flags & PTE_DIRTY))
    pte->lo |= PTE_DIRTY;

  klog("Change protection bits for page %08lx (PTE at %08lx)",
       (vaddr & PTE_MASK), (vm_addr_t)&PTE_OF(pmap, vaddr));

  /* invalidate corresponding entry in tlb */
  tlb_invalidate(PTE_VPN2(vaddr) | PTE_ASID(pmap->asid));
}

/*
 * Check if given virtual address is mapped according to TLB and page table.
 * Detects any inconsistencies.
 */
bool pmap_probe(pmap_t *pmap, vm_addr_t start, vm_addr_t end, vm_prot_t prot) {
  assert(is_aligned(start, PAGESIZE) && is_aligned(end, PAGESIZE));
  assert(start < end);

  if (start < pmap->start || end > pmap->end)
    return false;

  tlblo_t expected = vm_prot_map[prot];

  while (start < end) {
    pmap_assure_pde_in_tlb(pmap, start);
    tlbhi_t hi = PTE_VPN2(start) | PTE_ASID(pmap->asid);
    tlblo_t lo0, lo1;
    int tlb_idx = tlb_probe(hi, &lo0, &lo1);
    pte_t pte =
      pde_is_valid(PDE_OF(pmap, start)) ? PTE_OF(pmap, start) : (pte_t){0, 0};

    if (tlb_idx >= 0) {
      tlblo_t lo = PTE_LO_INDEX_OF(start) ? lo1 : lo0;
      if (lo != pte.lo)
        panic("TLB (%08x) vs. PTE (%08lx) mismatch for virtual address %08lx!",
              lo, pte.lo, start);
    }

    if (((pte.lo | pte.ext_flags) & PTE_PROT_MASK) != expected)
      return false;

    start += PAGESIZE;
  }

  return true;
}

void pmap_map(pmap_t *pmap, vm_addr_t start, vm_addr_t end, pm_addr_t paddr,
              vm_prot_t prot) {
  assert(is_aligned(start, PAGESIZE) && is_aligned(end, PAGESIZE));
  assert(start < end && start >= pmap->start && end <= pmap->end);
  assert(is_aligned(paddr, PAGESIZE));

  while (start < end) {
    pmap_set_pte(pmap, start, paddr, prot);
    start += PAGESIZE, paddr += PAGESIZE;
  }
}

void pmap_unmap(pmap_t *pmap, vm_addr_t start, vm_addr_t end) {
  assert(is_aligned(start, PAGESIZE) && is_aligned(end, PAGESIZE));
  assert(start < end && start >= pmap->start && end <= pmap->end);

  while (start < end) {
    pmap_clear_pte(pmap, start);
    start += PAGESIZE;
  }
}

void pmap_protect(pmap_t *pmap, vm_addr_t start, vm_addr_t end,
                  vm_prot_t prot) {
  assert(is_aligned(start, PAGESIZE) && is_aligned(end, PAGESIZE));
  assert(start < end && start >= pmap->start && end <= pmap->end);

  while (start < end) {
    pmap_change_pte(pmap, start, prot);
    start += PAGESIZE;
  }
}

/* TODO: at any given moment there're two page tables in use:
 *  - kernel-space pmap for kseg2 & kseg3
 *  - user-space pmap for useg
 * ks_pmap is shared across all vm_maps. How to switch us_pmap efficiently?
 * Correct solution needs to make sure no unwanted entries are left in TLB and
 * caches after the switch.
 */

void pmap_activate(pmap_t *pmap) {
  SCOPED_CRITICAL_SECTION();

  assert(pmap != &kernel_pmap);
  PCPU_GET(curpmap) = pmap;
  mips32_set_c0(C0_ENTRYHI, pmap ? pmap->asid : 0);
  if (pmap)
    pmap_setup_pd_wired_tlb_entry(pmap);
  /* thanks to ASIDs we can avoid TLB flushes */
}

pmap_t *get_kernel_pmap() {
  return &kernel_pmap;
}

pmap_t *get_user_pmap() {
  return PCPU_GET(curpmap);
}

pmap_t *get_active_pmap_by_addr(vm_addr_t addr) {
  if (in_kernel_space(addr))
    return get_kernel_pmap();
  if (in_user_space(addr))
    return get_user_pmap();
  return NULL;
}

void tlb_fault(exc_frame_t *frame) {
  thread_t *td = thread_self();

  if (td->td_onfault) {
    /* handle copyin / copyout faults */
    frame->pc = td->td_onfault;
    td->td_onfault = 0;
  } else if (td->td_proc) {
    /* Send a segmentation fault signal to the user program. */
    sig_send(td->td_proc, SIGSEGV);
  } else if (ktest_test_running_flag) {
    ktest_failure();
  } else {
    /* Kernel mode thread violated memory, whoops. */
    panic("Invalid memory access.");
  }
}

/* Returns true on success */
bool tlb_page_fault(vm_addr_t vaddr, vm_prot_t access) {
  vm_map_t *map = get_active_vm_map_by_addr(vaddr);
  if (!map) {
    klog("No virtual address space definde for $%08lx!", vaddr);
    return false;
  }
  if (vm_page_fault(map, vaddr, access) == 0)
    return true;

  return false;
}

/* Returns false if page fault is needed
 * vaddr - original page virtual address */
bool tlb_pde_refill(exc_frame_t *frame, vm_addr_t orig_vaddr) {
  pmap_t *pmap = get_active_pmap_by_addr(orig_vaddr);
  if (!pmap) {
    klog("Address $%08lx not mapped by any active pmap!", orig_vaddr);
    tlb_fault(frame);
  }

  pde_t *pde = &PDE_OF(pmap, orig_vaddr);
  if (pde_is_valid(*pde)) {
    klog("TLB refill for page table fragment $%08lx", orig_vaddr & PTE_MASK);
    vm_addr_t ptf_start = PTF_ADDR_OF(orig_vaddr);

    /* Both ENTRYLO0 and ENTRYLO1 must have G bit set for address translation
     * to skip ASID check. */
    tlb_overwrite_random(ptf_start | pmap->asid, pde->lo,
                         pde->lo | PTE_PFN(PAGESIZE));
    return true;
  }

  return false;
}

/* Returns false if page fault is needed */
bool tlb_pte_refill(exc_frame_t *frame, vm_addr_t vaddr) {
  pmap_t *pmap = get_active_pmap_by_addr(vaddr);
  if (!pmap) {
    klog("Address $%08lx not mapped by any active pmap!", vaddr);
    tlb_fault(frame);
  }

  if (pde_is_valid(PDE_OF(pmap, vaddr))) {
    pmap_assure_pde_in_tlb(pmap, vaddr);

    unsigned pte_idx = PTE_INDEX(vaddr);
    pte_t *pte = &pmap->pte[pte_idx];

    if (pte->ext_flags & PTE_VALID && !(pte->lo & PTE_VALID)) {
      /* Mark page as referenced */
      pte->lo |= PTE_VALID;
      pte->ext_flags |= PTE_EXT_REFERENCED;
    }
    if (pte->lo & PTE_VALID) {
      tlbhi_t hi = mips32_get_c0(C0_ENTRYHI);
      tlblo_t lo0 = pmap->pte[pte_idx & ~1].lo;
      tlblo_t lo1 = pmap->pte[pte_idx | 1].lo;
      tlb_overwrite_random(hi, lo0, lo1);
      return true;
    }
  }

  return false;
}

void tlb_load_exception_handler(exc_frame_t *frame) {
  assert(frame);

  vm_addr_t vaddr = (vm_addr_t)frame->badvaddr;

  /* Page table for KSEG0 and KSEG1 must not be queried, cause the address
   * range is not a subject to TLB based address translation. */
  assert(vaddr < PT_HOLE_START || vaddr >= PT_HOLE_END);

  /* Page directory must not be queried, cause its TLB entries
   * are supposed to be wired */
  assert(vaddr < KERNEL_PD_BASE || vaddr >= KERNEL_PD_BASE + PD_SIZE);
  assert(vaddr < USER_PD_BASE || vaddr >= USER_PD_BASE + PD_SIZE);

  /* First check if TLB contains any entry associated with desired address */
  tlblo_t lo0, lo1;
  tlbhi_t hi = mips32_get_c0(C0_ENTRYHI);
  int tlb_idx = tlb_probe(hi, &lo0, &lo1);

  if (tlb_idx >= 0) {
    /* If returned TLB index is not negative this means
     * that an entry is found but is invalid.
     * Check if it is indeed invalid or is only
     * marked as such in order to detect page references. */

    assert(tlb_idx >= TLB_WIRED_ENTRIES);

    if (vaddr >= PT_BASE && vaddr < PT_BASE + PT_SIZE) {
      /* We don't accept invalid TLB entries for page table fragments */
      vm_addr_t ptf_start = PTF_ADDR_OF(vaddr);
      klog("Found an invalid TLB entry for page table fragment at $%08lx!",
           ptf_start);
      tlb_fault(frame);
      return;
    }

    pmap_t *pmap = get_active_pmap_by_addr(vaddr);
    if (!pmap) {
      klog("Address $%08lx not mapped by any active pmap!", vaddr);
      tlb_fault(frame);
      return;
    }

    /* Check if respective PDE is valid */
    if (pde_is_valid(PDE_OF(pmap, vaddr))) {
      /* Check if respective PTE is valid */
      pmap_assure_pde_in_tlb(pmap, vaddr);
      unsigned pte_idx = PTE_INDEX(vaddr);
      pte_t *pte = &pmap->pte[pte_idx];
      if (pte->ext_flags & PTE_VALID) {
        /* PTE entry is valid. Mark page as referenced and update TLB */
        pte->lo |= PTE_VALID;
        pte->ext_flags |= PTE_EXT_REFERENCED;

        lo0 = pmap->pte[pte_idx & ~1].lo;
        lo1 = pmap->pte[pte_idx | 1].lo;

        tlb_write_index(hi, lo0, lo1, (unsigned)tlb_idx);
        return;
      }
    }

    /* Otherwise, page fault */
    if (tlb_page_fault(vaddr, VM_PROT_READ))
      return;

  } else {
    /* Otherwise, no TLB entry associated with vaddr is found
     * This means that either vaddr is from the PT range and
     * TLB refill occured for page table fragment
     * or TLB refill occured while handling an interrupt/exception */

    if (vaddr >= PT_BASE && vaddr < PT_BASE + PT_SIZE) {
      /* TLB refill for page table fragment
       * Assure PDE is valid and update it
       * Do not overwrite PTE, it will be done in subsequent PTE Refill */

      /* Restore the original address */
      vm_addr_t orig_vaddr = (vaddr - PT_BASE) / sizeof(pte_t) * PAGESIZE;
      assert(PT_BASE > orig_vaddr || orig_vaddr >= PT_BASE + PT_SIZE);

      if (tlb_pde_refill(frame, orig_vaddr))
        return;

      /* Otherwise, page fault */
      if (tlb_page_fault(orig_vaddr, VM_PROT_READ))
        return;

    } else {
      /* TLB refill during an exception/interrupt
       * Do not allow page faults here */

      if (tlb_pte_refill(frame, vaddr))
        return;

      klog("Page fault while in an interrupt/exception!");
    }
  }

  tlb_fault(frame);
}

void tlb_store_exception_handler(exc_frame_t *frame) {
  assert(frame);

  vm_addr_t vaddr = (vm_addr_t)frame->badvaddr;

  /* Page table for KSEG0 and KSEG1 must not be queried, cause the address
   * range is not a subject to TLB based address translation. */
  assert(vaddr < PT_HOLE_START || vaddr >= PT_HOLE_END);

  /* Page directory must not be queried, cause its TLB entries
   * are supposed to be wired */
  assert(vaddr < KERNEL_PD_BASE || vaddr >= KERNEL_PD_BASE + PD_SIZE);
  assert(vaddr < USER_PD_BASE || vaddr >= USER_PD_BASE + PD_SIZE);

  /* First check if TLB contains any entry associated with desired address */
  tlblo_t lo0, lo1;
  tlbhi_t hi = mips32_get_c0(C0_ENTRYHI);
  int tlb_idx = tlb_probe(hi, &lo0, &lo1);

  if (tlb_idx >= 0) {
    /* If returned TLB index is not negative this means
     * that an entry is found but is invalid.
     * Check if it is indeed invalid or is only
     * marked as such in order to detect page references. */

    assert(tlb_idx >= TLB_WIRED_ENTRIES);

    if (vaddr >= PT_BASE && vaddr < PT_BASE + PT_SIZE) {
      /* We don't accept invalid TLB entries for page table fragments */
      vm_addr_t ptf_start = PTF_ADDR_OF(vaddr);
      klog("Found an invalid TLB entry for page table fragment at $%08lx!",
           ptf_start);
      tlb_fault(frame);
      return;
    }

    pmap_t *pmap = get_active_pmap_by_addr(vaddr);
    if (!pmap) {
      klog("Address $%08lx not mapped by any active pmap!", vaddr);
      tlb_fault(frame);
      return;
    }

    /* Check if respective PDE is valid */
    if (pde_is_valid(PDE_OF(pmap, vaddr))) {
      /* Check if respective PTE is valid and writable */
      pmap_assure_pde_in_tlb(pmap, vaddr);
      unsigned pte_idx = PTE_INDEX(vaddr);
      pte_t *pte = &pmap->pte[pte_idx];
      if (pte->ext_flags & PTE_VALID && pte->ext_flags & PTE_DIRTY) {
        /* PTE entry is valid and dirty.
         * Mark page as referenced and modified, then update TLB */
        pte->lo |= PTE_VALID | PTE_DIRTY;
        pte->ext_flags |= PTE_EXT_REFERENCED | PTE_EXT_MODIFIED;

        lo0 = pmap->pte[pte_idx & ~1].lo;
        lo1 = pmap->pte[pte_idx | 1].lo;

        tlb_write_index(hi, lo0, lo1, (unsigned)tlb_idx);
        return;
      }
    }

    /* Otherwise, page fault */
    if (tlb_page_fault(vaddr, VM_PROT_READ))
      return;

  } else {
    /* Otherwise, no TLB entry associated with vaddr is found
     * This means that either vaddr is from the PT range and
     * TLB refill occured for page table fragment (which is not possible)
     * or TLB refill occured while handling an interrupt/exception */

    assert(vaddr < PT_BASE || vaddr >= PT_BASE + PT_SIZE);
    /* TLB refill during an exception/interrupt
     * Do not allow page faults here */

    if (tlb_pte_refill(frame, vaddr))
      return;

    klog("Page fault while in an interrupt/exception!");
  }

  tlb_fault(frame);
}

void tlb_modified_exception_handler(exc_frame_t *frame) {
  assert(frame);
  /* We are sure here that respective TLB entry resides in TLB */

  vm_addr_t vaddr = (vm_addr_t)frame->badvaddr;

  /* First get TLB index containing an entry associated with desired address */
  tlblo_t lo0, lo1;
  tlbhi_t hi = mips32_get_c0(C0_ENTRYHI);
  int tlb_idx = tlb_probe(hi, &lo0, &lo1);
  assert(tlb_idx >= 0);

  pmap_t *pmap = get_active_pmap_by_addr(vaddr);

  pmap_assure_pde_in_tlb(pmap, vaddr);

  /* Check if respective PTE is indeed dirty
   * Otherwise, page fault */

  unsigned pte_idx = PTE_INDEX(vaddr);
  pte_t *pte = &pmap->pte[pte_idx];
  if (pte->ext_flags & PTE_DIRTY) {
    /* PTE entry is dirty. Mark page as modified and update TLB */
    pte->lo |= PTE_DIRTY;
    pte->ext_flags |= PTE_EXT_MODIFIED;

    lo0 = pmap->pte[pte_idx & ~1].lo;
    lo1 = pmap->pte[pte_idx | 1].lo;

    tlb_write_index(hi, lo0, lo1, (unsigned)tlb_idx);
    return;
  }

  if (tlb_page_fault(vaddr, VM_PROT_WRITE))
    return;

  tlb_fault(frame);
}

SYSINIT_ADD(pmap, pmap_init, NODEPS);
