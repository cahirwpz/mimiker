#include <stdc.h>
#include <malloc.h>
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

#define PD_ENTRIES 1024  /* page directory entries */
#define PD_SIZE (PD_ENTRIES * sizeof(pte_t))
#define PTF_ENTRIES 1024 /* page table fragment entries */
#define PTF_SIZE (PTF_ENTRIES * sizeof(pte_t))
#define PT_ENTRIES (PD_ENTRIES * PTF_ENTRIES)
#define PT_SIZE (PT_ENTRIES * sizeof(pte_t))
#define UNMAPPED_PFN 0x00000000

static pmap_t *active_pmap[2];

typedef struct {
  vm_addr_t start, end;
} pmap_range_t;

static pmap_range_t pmap_range[PMAP_LAST] = {
  [PMAP_KERNEL] = {0xc0000000 + PT_SIZE, 0xfffff000}, /* kseg2 & kseg3 */
  [PMAP_USER] = {0x00000000, 0x80000000}, /* useg */
};

void pmap_setup(pmap_t *pmap, pmap_type_t type, asid_t asid) {
  pmap->type = type;
  pmap->pte = (pte_t *)PT_BASE;
  pmap->pde_page = pm_alloc(1);
  pmap->pde = (pte_t *)pmap->pde_page->vaddr;
  pmap->start = pmap_range[type].start;
  pmap->end = pmap_range[type].end;
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

bool pmap_mapped_page(pmap_t *pmap, vm_addr_t vaddr) {
  assert(is_aligned(vaddr, PAGESIZE));
  if (PDE_OF(pmap, vaddr) & PTE_VALID)
    if (PTE_OF(pmap, vaddr) & PTE_VALID)
      return true;
  return false;
}

/* TODO: implement in an efficient manner */
bool pmap_mapped_page_range(pmap_t *pmap, vm_addr_t start, vm_addr_t end);

#define PTF_ADDR_OF(vaddr) (PT_BASE + PDE_INDEX(vaddr) * PAGESIZE)

/* Add PT to PD so kernel can handle access to @vaddr. */
static void pmap_add_pde(pmap_t *pmap, vm_addr_t vaddr) {
  /* assume page table fragment not present in physical memory */
  assert (!(PDE_OF(pmap, vaddr) & PTE_VALID));

  vm_page_t *pg = pm_alloc(1);
  TAILQ_INSERT_TAIL(&pmap->pte_pages, pg, pt.list);
  log("Page table fragment %08lx allocated at %08lx", 
      PTF_ADDR_OF(vaddr), pg->paddr);

  PDE_OF(pmap, vaddr) = PTE_PFN(pg->paddr) | PTE_VALID | PTE_DIRTY | PTE_GLOBAL;

  uint32_t pde_index = PDE_INDEX(vaddr) & ~1;
  vm_addr_t ptf_start = PT_BASE + pde_index * PAGESIZE;

  /* Both ENTRYLO0 and ENTRYLO1 must have G bit set for address translation to
   * skip ASID check. */
  tlb_overwrite_random(ptf_start | pmap->asid,
                       pmap->pde[pde_index],
                       pmap->pde[pde_index + 1]);
  tlb_print();

  pte_t *pte = (pte_t *)PTF_ADDR_OF(vaddr);
  for (int i = 0; i < PTF_ENTRIES; i++)
    pte[i] = PTE_GLOBAL;
}

/* TODO: implement */
void pmap_remove_pde(pmap_t *pmap, uint32_t vaddr);

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

static void pmap_set_pte(pmap_t *pmap, vm_addr_t vaddr, pm_addr_t paddr,
                         vm_prot_t prot) {
  if (!(PDE_OF(pmap, vaddr) & PTE_VALID))
    pmap_add_pde(pmap, vaddr);

  PTE_OF(pmap, vaddr) = PTE_PFN(paddr) | vm_prot_map[prot] |
    (pmap->type == PMAP_KERNEL ? PTE_GLOBAL : 0);
  log("Add mapping for page %08lx (PTE at %08lx)",
      (vaddr & PTE_MASK), (intptr_t)&PTE_OF(pmap, vaddr));

  /* invalidate corresponding entry in tlb */
  tlb_invalidate(PTE_VPN2(vaddr) | PTE_ASID(pmap->asid));

  /* TODO: what about caches? */
}

/* TODO: implement */
void pmap_clear_pte(pmap_t *pmap, vm_addr_t vaddr) {
  assert(PDE_OF(pmap, vaddr) & PTE_VALID);

  pte_t pte = PTE_OF(pmap, vaddr);

  assert(pte & PTE_VALID);

#if 0
  if (pte & PTE_NO_EXEC)
    mips_clean_dcache(vaddr, PAGESIZE);
  else
    mips_clean_icache(vaddr, PAGESIZE);
#endif

  PTE_OF(pmap, vaddr) = 0;
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
    pte_t pte = (PDE_OF(pmap, start) & PTE_VALID) ? PTE_OF(pmap, start) : 0;

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

/* TODO: Fail noisly on valid PTE overwrite. User must pmap_unmap first! */
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
  while(start < end) {
    pmap_set_pte(pmap, start, UNMAPPED_PFN, VM_PROT_NONE);
    start += PAGESIZE;
  }
}

void pmap_protect(pmap_t *pmap, vm_addr_t start, vm_addr_t end,
                  vm_prot_t prot) {
  assert(is_aligned(start, PAGESIZE) && is_aligned(end, PAGESIZE));
  assert(start < end && start >= pmap->start && end <= pmap->end);
  while(start < end) {
    if(!(PDE_OF(pmap, start) & PTE_VALID)) {
      pmap_add_pde(pmap, start);
    }
    pte_t pte = PTE_OF(pmap, start);
    pm_addr_t paddr = PTE_PFN_OF(pte);
    pmap_set_pte(pmap, start, paddr, prot);
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
pmap_t *pmap_switch(size_t space, pmap_t *pmap);

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
  addr &= PTE_MASK;

  for (pmap_type_t type = 0; type < PMAP_LAST; type++)
    if (pmap_range[type].start >= addr && addr <= pmap_range[type].end)
      return active_pmap[type];

  return NULL;
}

void tlb_exception_handler() {
  int code = (mips32_get_c0(C0_CAUSE) & CR_X_MASK) >> CR_X_SHIFT;
  vm_addr_t vaddr = mips32_get_c0(C0_BADVADDR);

  log("%s at %08lx, caused by reference to %08lx!",
      exceptions[code], mips32_get_c0(C0_EPC), vaddr);
  tlb_print();

  /* If the fault was in virtual pt range it means it's time to refill */
  if (PT_BASE <= vaddr && vaddr < PT_BASE + PT_SIZE) {
    uint32_t index = ((vaddr & PTE_MASK) - PT_BASE) >> PTE_SHIFT;
    log("TLB refill for page table fragment %08lx", vaddr & PTE_MASK);

    pmap_t *pmap = get_active_pmap_by_addr(vaddr);
    if (!pmap)
      panic("Address %08lx not mapped by active pmap!", vaddr);
    if (!(pmap->pde[index] & PTE_VALID))
      panic("Unmapped page table for address %08lx!", vaddr);

    index &= ~1;

    vm_addr_t ptf_start = PT_BASE + index * PAGESIZE;
    tlbhi_t entryhi = ptf_start | pmap->asid;
    tlblo_t entrylo0 = pmap->pde[index];
    tlblo_t entrylo1 = pmap->pde[index + 1];
    tlb_overwrite_random(entryhi, entrylo0, entrylo1);
  } else {
    vm_map_t *map =
      get_active_vm_map(vaddr < 0x80000000 ? PMAP_USER : PMAP_KERNEL);
    if (!map)
      panic("No virtual address space defined for %08lx!", vaddr);
    vm_page_fault(map, vaddr, code == EXC_TLBL ? VM_PROT_READ : VM_PROT_WRITE);
  }
}
