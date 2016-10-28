#include <stdc.h>
#include <malloc.h>
#include <mips/exc.h>
#include <mips/mips.h>
#include <mips/tlb.h>
#include <pmap.h>
#include <vm_map.h>

#define PTE_MASK 0xfffff000
#define PTE_SHIFT 12
#define PDE_MASK 0xffc00000
#define PDE_SHIFT 22

#define PTE_INDEX(x) (((x) & PTE_MASK) >> PTE_SHIFT)
#define PDE_INDEX(x) (((x) & PDE_MASK) >> PDE_SHIFT)

#define PTE_OF(pmap, addr) ((pmap)->pte[PTE_INDEX(addr)])
#define PDE_OF(pmap, addr) ((pmap)->pde[PDE_INDEX(addr)])
#define PTF_ADDR_OF(vaddr) (PT_BASE + PDE_INDEX(vaddr) * PAGESIZE)

#define PD_ENTRIES 1024  /* page directory entries */
#define PD_SIZE (PD_ENTRIES * sizeof(pte_t))
#define PTF_ENTRIES 1024 /* page table fragment entries */
#define PTF_SIZE (PTF_ENTRIES * sizeof(pte_t))
#define PT_ENTRIES (PD_ENTRIES * PTF_ENTRIES)
#define PT_SIZE (PT_ENTRIES * sizeof(pte_t))

static const vm_addr_t PT_HOLE_START =
  PT_BASE + MIPS_KSEG0_START / PTF_ENTRIES;
static const vm_addr_t PT_HOLE_END =
  PT_BASE + MIPS_KSEG2_START / PTF_ENTRIES;

static bool is_valid(pte_t pte) {
  return pte & PTE_VALID;
}

static pmap_t *active_pmap[PMAP_LAST];

typedef struct {
  vm_addr_t start, end;
} pmap_range_t;

static pmap_range_t pmap_range[PMAP_LAST] = {
  [PMAP_KERNEL] = {MIPS_KSEG2_START + PT_SIZE, 0xfffff000}, /* kseg2 & kseg3 */
  [PMAP_USER] = {0x00000000, MIPS_KSEG0_START} /* useg */
};

void pmap_setup(pmap_t *pmap, pmap_type_t type, asid_t asid) {
  pmap->type = type;
  pmap->pte = (pte_t *)PT_BASE;
  pmap->pde_page = pm_alloc(1);
  pmap->pde = (pte_t *)pmap->pde_page->vaddr;
  pmap->start = pmap_range[type].start;
  pmap->end = pmap_range[type].end;
  pmap->asid = asid;
  log("Page directory table allocated at %08lx", (intptr_t)pmap->pde);
  TAILQ_INIT(&pmap->pte_pages);

  for (int i = 0; i < PD_ENTRIES; i++)
    pmap->pde[i] = (pmap->type == PMAP_KERNEL) ? PTE_GLOBAL : 0;
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
  assert (!is_valid(PDE_OF(pmap, vaddr)));

  vm_page_t *pg = pm_alloc(1);
  TAILQ_INSERT_TAIL(&pmap->pte_pages, pg, pt.list);
  log("Page table fragment %08lx allocated at %08lx", 
      PTF_ADDR_OF(vaddr), pg->paddr);

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
  [VM_PROT_WRITE] = PTE_VALID|PTE_DIRTY,
  [VM_PROT_READ|VM_PROT_WRITE] = PTE_VALID|PTE_DIRTY,
  [VM_PROT_EXEC] = PTE_VALID,
  [VM_PROT_READ|VM_PROT_EXEC] = PTE_VALID,
  [VM_PROT_WRITE|VM_PROT_EXEC] = PTE_VALID|PTE_DIRTY,
  [VM_PROT_READ|VM_PROT_WRITE|VM_PROT_EXEC] = PTE_VALID|PTE_DIRTY,
};
#endif

/* TODO: what about caches? */
static void pmap_set_pte(pmap_t *pmap, vm_addr_t vaddr, pm_addr_t paddr,
                         vm_prot_t prot) {
  if (!is_valid(PDE_OF(pmap, vaddr)))
    pmap_add_pde(pmap, vaddr);

  PTE_OF(pmap, vaddr) = PTE_PFN(paddr) | vm_prot_map[prot] |
    (pmap->type == PMAP_KERNEL ? PTE_GLOBAL : 0);
  log("Add mapping for page %08lx (PTE at %08lx)",
      (vaddr & PTE_MASK), (intptr_t)&PTE_OF(pmap, vaddr));

  /* invalidate corresponding entry in tlb */
  tlb_invalidate(PTE_VPN2(vaddr) | PTE_ASID(pmap->asid));
}

/* TODO: what about caches? */
static void pmap_clear_pte(pmap_t *pmap, vm_addr_t vaddr) {
  PTE_OF(pmap, vaddr) = 0;
  log("Remove mapping for page %08lx (PTE at %08lx)",
      (vaddr & PTE_MASK), (intptr_t)&PTE_OF(pmap, vaddr));
  /* invalidate corresponding entry in tlb */
  tlb_invalidate(PTE_VPN2(vaddr) | PTE_ASID(pmap->asid));

  /* TODO: Deallocate empty page table fragment by calling pmap_remove_pde. */
}

/* TODO: what about caches? */
static void pmap_change_pte(pmap_t *pmap, vm_addr_t vaddr, vm_prot_t prot) {
  PTE_OF(pmap, vaddr) = 
    (PTE_OF(pmap, vaddr) & ~PTE_PROT_MASK) | vm_prot_map[prot];
  log("Change protection bits for page %08lx (PTE at %08lx)",
      (vaddr & PTE_MASK), (intptr_t)&PTE_OF(pmap, vaddr));

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
pmap_t *pmap_switch(pmap_t *pmap) {
  pmap_t *old_pmap = active_pmap[PMAP_USER];
  set_active_pmap(pmap);
  return old_pmap;
}

void set_active_pmap(pmap_t *pmap) {
  pmap_type_t type = pmap->type;

  if (type == PMAP_KERNEL && active_pmap[type])
    panic("Kernel page table can be set only once!");

  active_pmap[type] = pmap;
  mips32_set_c0(C0_ENTRYHI, pmap->asid);
}

pmap_t *get_active_pmap(pmap_type_t type) {
  assert(type == PMAP_KERNEL || type == PMAP_USER);
  return active_pmap[type];
}

pmap_t *get_active_pmap_by_addr(vm_addr_t addr) {
  for (pmap_type_t type = 0; type < PMAP_LAST; type++)
    if (pmap_range[type].start <= addr && addr < pmap_range[type].end)
      return active_pmap[type];

  return NULL;
}

void tlb_exception_handler(exc_frame_t *frame) {
  int code = (mips32_get_c0(C0_CAUSE) & CR_X_MASK) >> CR_X_SHIFT;
  vm_addr_t vaddr = mips32_get_c0(C0_BADVADDR);

  log("%s at %08lx, caused by reference to %08lx!",
      exceptions[code], mips32_get_c0(C0_EPC), vaddr);
  tlb_print();

  /* If the fault was in virtual pt range it means it's time to refill */
  if (PT_BASE <= vaddr && vaddr < PT_BASE + PT_SIZE) {
    uint32_t index = PTE_INDEX(vaddr - PT_BASE) & ~1;
    vm_addr_t orig_vaddr = (vaddr - PT_BASE) * PTF_ENTRIES;

    assert(vaddr < PT_HOLE_START || vaddr >= PT_HOLE_END);

    if (PT_BASE <= orig_vaddr && orig_vaddr < PT_BASE + PT_SIZE) {
      /* TLB refill exception can occur within interrupt handler, we have to be
       * prepared for it! */
      vaddr = orig_vaddr;
      index = PTE_INDEX(vaddr - PT_BASE) & ~1;
      orig_vaddr = (vaddr - PT_BASE) * PTF_ENTRIES;
    }

    pmap_t *pmap = get_active_pmap_by_addr(orig_vaddr);
    if (!pmap)
      panic("Address %08lx not mapped by any active pmap!", orig_vaddr);
    if (is_valid(pmap->pde[index]) || is_valid(pmap->pde[index + 1])) {
      log("TLB refill for page table fragment %08lx", vaddr & PTE_MASK);
      vm_addr_t ptf_start = PT_BASE + index * PAGESIZE;

      /* Both ENTRYLO0 and ENTRYLO1 must have G bit set for address translation
       * to skip ASID check. */
      tlb_overwrite_random(ptf_start | pmap->asid,
                           pmap->pde[index],
                           pmap->pde[index + 1]);
      return;
    }

    /* We needed to refill TLB, but the address is not mapped yet!
     * Forward the request to pager */
    vaddr = orig_vaddr;
  }

  vm_map_t *map = get_active_vm_map_by_addr(vaddr);
  if (!map)
    panic("No virtual address space defined for %08lx!", vaddr);
  vm_page_fault(map, vaddr, code == EXC_TLBL ? VM_PROT_READ : VM_PROT_WRITE);
}
