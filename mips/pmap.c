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
#include <sysinit.h>

static MALLOC_DEFINE(M_PMAP, "pmap", 1, 1);

#define PTE_MASK 0xfffff000
#define PTE_SHIFT 12
#define PDE_MASK 0xffc00000
#define PDE_SHIFT 22

#define PTE_INDEX(x) (((x)&PTE_MASK) >> PTE_SHIFT)
#define PDE_INDEX(x) (((x)&PDE_MASK) >> PDE_SHIFT)

#define PTE_OF(pmap, addr) ((pmap)->pte[PTE_INDEX(addr)])
#define PDE_OF(pmap, addr) ((pmap)->pde[PDE_INDEX(addr)])
#define PTF_ADDR_OF(vaddr) (PT_BASE + PDE_INDEX(vaddr) * PAGESIZE)

#define PD_ENTRIES 1024 /* page directory entries */
#define PD_SIZE (PD_ENTRIES * sizeof(pte_t))
#define PTF_ENTRIES 1024 /* page table fragment entries */
#define PTF_SIZE (PTF_ENTRIES * sizeof(pte_t))
#define PT_ENTRIES (PD_ENTRIES * PTF_ENTRIES)
#define PT_SIZE (PT_ENTRIES * sizeof(pte_t))

#define PMAP_KERNEL_BEGIN MIPS_KSEG2_START
#define PMAP_KERNEL_END 0xfffff000 /* kseg2 & kseg3 */
#define PMAP_USER_BEGIN 0x00000000
#define PMAP_USER_END MIPS_KSEG0_START /* useg */

static const vm_addr_t PT_HOLE_START = PT_BASE + MIPS_KSEG0_START / PTF_ENTRIES;
static const vm_addr_t PT_HOLE_END = PT_BASE + MIPS_KSEG2_START / PTF_ENTRIES;

static bool is_valid(pte_t pte) {
  return pte & PTE_VALID;
}

static bool in_user_space(vm_addr_t addr) {
  return (addr >= PMAP_USER_BEGIN && addr < PMAP_USER_END);
}

static bool in_kernel_space(vm_addr_t addr) {
  return (addr >= PMAP_KERNEL_BEGIN && addr < PMAP_KERNEL_END);
}

static pmap_t kernel_pmap;
static asid_t asid_counter;

static asid_t get_new_asid() {
  if (asid_counter < MAX_ASID)
    return asid_counter++; // TODO this needs to be atomic increment
  else
    panic("Out of asids!");
}

static void pmap_setup(pmap_t *pmap, vm_addr_t start, vm_addr_t end) {
  pmap->pte = (pte_t *)PT_BASE;
  pmap->pde_page = pm_alloc(1);
  pmap->pde = (pte_t *)pmap->pde_page->vaddr;
  pmap->start = start;
  pmap->end = end;
  pmap->asid = get_new_asid();
  klog("Page directory table allocated at %08lx", (vm_addr_t)pmap->pde);
  TAILQ_INIT(&pmap->pte_pages);

  for (int i = 0; i < PD_ENTRIES; i++)
    pmap->pde[i] = in_kernel_space(i * PTF_ENTRIES * PAGESIZE) ? PTE_GLOBAL : 0;
}

/* TODO: remove all mappings from TLB, evict related cache lines */
void pmap_reset(pmap_t *pmap) {
  while (!TAILQ_EMPTY(&pmap->pte_pages)) {
    vm_page_t *pg = TAILQ_FIRST(&pmap->pte_pages);
    TAILQ_REMOVE(&pmap->pte_pages, pg, pt.list);
    pm_free(pg);
  }
  pm_free(pmap->pde_page);
  memset(pmap, 0, sizeof(pmap_t)); /* Set up for reuse. */
}

static void pmap_init() {
  pmap_setup(&kernel_pmap, PMAP_KERNEL_BEGIN + PT_SIZE, PMAP_KERNEL_END);
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

bool pmap_is_mapped(pmap_t *pmap, vm_addr_t vaddr) {
  assert(is_aligned(vaddr, PAGESIZE));
  if (is_valid(PDE_OF(pmap, vaddr)))
    if (is_valid(PTE_OF(pmap, vaddr)))
      return true;
  return false;
}

bool pmap_is_range_mapped(pmap_t *pmap, vm_addr_t start, vm_addr_t end) {
  assert(is_aligned(start, PAGESIZE) && is_aligned(end, PAGESIZE));
  assert(start < end);

  vm_addr_t addr;

  for (addr = start; addr < end; addr += PD_ENTRIES * PAGESIZE)
    if (!is_valid(PDE_OF(pmap, addr)))
      return false;

  for (addr = start; addr < end; addr += PTF_ENTRIES * PAGESIZE)
    if (!is_valid(PTE_OF(pmap, addr)))
      return false;

  return true;
}

/* Add PT to PD so kernel can handle access to @vaddr. */
static void pmap_add_pde(pmap_t *pmap, vm_addr_t vaddr) {
  /* assume page table fragment not present in physical memory */
  assert(!is_valid(PDE_OF(pmap, vaddr)));

  vm_page_t *pg = pm_alloc(1);
  TAILQ_INSERT_TAIL(&pmap->pte_pages, pg, pt.list);
  klog("Page table fragment %08lx allocated at %08lx", PTF_ADDR_OF(vaddr),
       pg->paddr);

  PDE_OF(pmap, vaddr) = PTE_PFN(pg->paddr) | PTE_VALID | PTE_DIRTY | PTE_GLOBAL;

  pte_t *pte = (pte_t *)PTF_ADDR_OF(vaddr);
  for (int i = 0; i < PTF_ENTRIES; i++)
    pte[i] = PTE_GLOBAL;
}

/* TODO: implement */
void pmap_remove_pde(pmap_t *pmap, vm_addr_t vaddr);

#if 0
/* Used if CPU implements RI and XI bits in ENTRYLO. */
static pte_t vm_prot_map[] = {
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
static pte_t vm_prot_map[] = {
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
  if (!is_valid(PDE_OF(pmap, vaddr)))
    pmap_add_pde(pmap, vaddr);

  PTE_OF(pmap, vaddr) = PTE_PFN(paddr) | vm_prot_map[prot] |
                        (in_kernel_space(vaddr) ? PTE_GLOBAL : 0);
  klog("Add mapping for page %08lx (PTE at %08lx)", (vaddr & PTE_MASK),
       (vm_addr_t)&PTE_OF(pmap, vaddr));

  /* invalidate corresponding entry in tlb */
  tlb_invalidate(PTE_VPN2(vaddr) | PTE_ASID(pmap->asid));
}

/* TODO: what about caches? */
static void pmap_clear_pte(pmap_t *pmap, vm_addr_t vaddr) {
  PTE_OF(pmap, vaddr) = 0;
  klog("Remove mapping for page %08lx (PTE at %08lx)", (vaddr & PTE_MASK),
       (vm_addr_t)&PTE_OF(pmap, vaddr));
  /* invalidate corresponding entry in tlb */
  tlb_invalidate(PTE_VPN2(vaddr) | PTE_ASID(pmap->asid));

  /* TODO: Deallocate empty page table fragment by calling pmap_remove_pde. */
}

/* TODO: what about caches? */
static void pmap_change_pte(pmap_t *pmap, vm_addr_t vaddr, vm_prot_t prot) {
  PTE_OF(pmap, vaddr) =
    (PTE_OF(pmap, vaddr) & ~PTE_PROT_MASK) | vm_prot_map[prot];
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

  pte_t expected = vm_prot_map[prot];

  while (start < end) {
    tlbhi_t hi = PTE_VPN2(start) | PTE_ASID(pmap->asid);
    tlblo_t lo0, lo1;
    int tlb_idx = tlb_probe(hi, &lo0, &lo1);
    pte_t pte = is_valid(PDE_OF(pmap, start)) ? PTE_OF(pmap, start) : 0;

    if (tlb_idx >= 0) {
      tlblo_t lo = PTE_LO_INDEX_OF(start) ? lo1 : lo0;
      if (lo != pte)
        panic("TLB (%08x) vs. PTE (%08lx) mismatch for virtual address %08lx!",
              lo, pte, start);
    }

    if ((pte & PTE_PROT_MASK) != expected)
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
  assert(!pmap_is_range_mapped(pmap, start, end));

  while (start < end) {
    pmap_set_pte(pmap, start, paddr, prot);
    start += PAGESIZE, paddr += PAGESIZE;
  }
}

void pmap_unmap(pmap_t *pmap, vm_addr_t start, vm_addr_t end) {
  assert(is_aligned(start, PAGESIZE) && is_aligned(end, PAGESIZE));
  assert(start < end && start >= pmap->start && end <= pmap->end);
  assert(pmap_is_range_mapped(pmap, start, end));

  while (start < end) {
    pmap_clear_pte(pmap, start);
    start += PAGESIZE;
  }
}

void pmap_protect(pmap_t *pmap, vm_addr_t start, vm_addr_t end,
                  vm_prot_t prot) {
  assert(is_aligned(start, PAGESIZE) && is_aligned(end, PAGESIZE));
  assert(start < end && start >= pmap->start && end <= pmap->end);
  assert(pmap_is_range_mapped(pmap, start, end));

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
  critical_enter();
  PCPU_GET(curpmap) = pmap;
  mips32_set_c0(C0_ENTRYHI, pmap ? pmap->asid : 0);
  critical_leave();
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

void tlb_exception_handler(exc_frame_t *frame) {
  thread_t *td = thread_self();

  int code = (frame->cause & CR_X_MASK) >> CR_X_SHIFT;
  vm_addr_t vaddr = frame->badvaddr;

  klog("%s at $%08x, caused by reference to $%08lx!", exceptions[code],
       frame->pc, vaddr);

  /* If the fault was in virtual pt range it means it's time to refill */
  if (PT_BASE <= vaddr && vaddr < PT_BASE + PT_SIZE) {
    uint32_t index = PTE_INDEX(vaddr - PT_BASE);
    /* Restore address that caused a TLB miss into virtualized page table. */
    vm_addr_t orig_vaddr = (vaddr - PT_BASE) * PTF_ENTRIES;

    /* Page table for KSEG0 and KSEG1 must not be queried, cause the address
     * range is not a subject to TLB based address translation. */
    assert(vaddr < PT_HOLE_START || vaddr >= PT_HOLE_END);

    if (PT_BASE <= orig_vaddr && orig_vaddr < PT_BASE + PT_SIZE) {
      /*
       * TLB refill exception can occur while C0_STATUS.EXL is set, if so then
       * TLB miss went directly to tlb_exception_handler instead of tlb_refill.
       */
      vaddr = orig_vaddr;
      index = PTE_INDEX(vaddr - PT_BASE);
      orig_vaddr = (vaddr - PT_BASE) * PTF_ENTRIES;
    }

    pmap_t *pmap = get_active_pmap_by_addr(orig_vaddr);
    if (!pmap) {
      klog("Address %08lx not mapped by any active pmap!", orig_vaddr);
      goto fault;
    }
    if (is_valid(pmap->pde[index])) {
      klog("TLB refill for page table fragment %08lx", vaddr & PTE_MASK);
      uint32_t index0 = index & ~1;
      uint32_t index1 = index0 | 1;
      vm_addr_t ptf_start = PT_BASE + index0 * PAGESIZE;

      /* Both ENTRYLO0 and ENTRYLO1 must have G bit set for address translation
       * to skip ASID check. */
      tlb_overwrite_random(ptf_start | pmap->asid, pmap->pde[index0],
                           pmap->pde[index1]);
      return;
    }

    /* We needed to refill TLB, but the address is not mapped yet!
     * Forward the request to pager */
    vaddr = orig_vaddr;
  }

  vm_map_t *map = get_active_vm_map_by_addr(vaddr);
  if (!map) {
    klog("No virtual address space defined for %08lx!", vaddr);
    goto fault;
  }
  vm_prot_t access = (code == EXC_TLBL) ? VM_PROT_READ : VM_PROT_WRITE;
  if (vm_page_fault(map, vaddr, access) == 0)
    return;

fault:
  /* handle copyin / copyout faults */
  if (td->td_onfault) {
    frame->pc = td->td_onfault;
    td->td_onfault = 0;
  } else if (ktest_test_running_flag) {
    ktest_failure();
  } else {
    thread_exit(-1);
  }
}

SYSINIT_ADD(pmap, pmap_init, NODEPS);
