#include <pmap.h>
#include <malloc.h>
#include <libkern.h>
#include <interrupts.h>
#include <tlb.h>
#include <vm_map.h>

#define PTE_MASK 0xfffff000
#define PTE_SHIFT 12
#define PDE_MASK 0xffc00000
#define PDE_SHIFT 22

#define PTE_INDEX(x) (((x) & PTE_MASK) >> PTE_SHIFT)
#define PDE_INDEX(x) (((x) & PDE_MASK) >> PDE_SHIFT)

#define PTE_OF(pmap, addr) ((pmap)->pte[PTE_INDEX(addr)])
#define PDE_OF(pmap, addr) ((pmap)->pde[PDE_INDEX(addr)])

#define PDE_ENTRIES 1024
#define PTE_ENTRIES (PDE_ENTRIES * 1024)
#define PTE_SIZE (PTE_ENTRIES * sizeof(pte_t))

static pmap_t *active_pmap[2];

typedef struct {
  vm_addr_t start, end;
} pmap_range_t;

static pmap_range_t pmap_range[] = {
  [PMAP_KERNEL] = {0xc0000000 + PTE_SIZE, 0xe0000000}, /* kseg2 */
  [PMAP_USER] = {0x00000000, 0x80000000}, /* useg */
};

void pmap_setup(pmap_t *pmap, pmap_type_t type, asid_t asid) {
  pmap->type = type;
  pmap->pte = (pte_t *)PTE_BASE;
  pmap->pde_page = pm_alloc(1);
  pmap->pde = (pte_t *)pmap->pde_page->vaddr;
  pmap->start = pmap_range[type].start;
  pmap->end = pmap_range[type].end;
  log("Page directory table allocated at %08lx", (intptr_t)pmap->pde);
  TAILQ_INIT(&pmap->pte_pages);
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

static void pmap_add_pde(pmap_t *pmap, vm_addr_t vaddr) {
  /* assume page table fragment not present in physical memory */
  assert (!(PDE_OF(pmap, vaddr) & PTE_VALID));

  vm_page_t *pg = pm_alloc(1);
  TAILQ_INSERT_TAIL(&pmap->pte_pages, pg, pt.list);
  log("Page table for (%08lx,%08lx) allocated at %08lx", 
      vaddr & PDE_MASK, (vaddr & PDE_MASK) + PTE_SIZE, pg->paddr);

  PDE_OF(pmap, vaddr) = PTE_PFN(pg->paddr)|PTE_VALID|PTE_DIRTY|PTE_GLOBAL;

  /* make sure proper address is in tlb */
  uint32_t pde_index = PDE_INDEX(vaddr) & ~1;
  vm_addr_t pte_start = PTE_BASE + pde_index * PAGESIZE;
  tlbhi_t entryhi = pte_start | pmap->asid;
  tlblo_t entrylo0 = pmap->pde[pde_index];
  tlblo_t entrylo1 = pmap->pde[pde_index + 1];
  tlb_overwrite_random(entryhi, entrylo0, entrylo1);
  tlb_print();

  memset((void *)pte_start, 0, PAGESIZE);
}

/* TODO: implement */
void pmap_remove_pde(pmap_t *pmap, uint32_t vaddr);

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

static void pmap_set_pte(pmap_t *pmap, vm_addr_t vaddr, pm_addr_t paddr,
                         vm_prot_t prot) {
  if (!(PDE_OF(pmap, vaddr) & PTE_VALID))
    pmap_add_pde(pmap, vaddr);

  PTE_OF(pmap, vaddr) = PTE_PFN(paddr) | vm_prot_map[prot] |
    (pmap->type == PMAP_KERNEL ? PTE_GLOBAL : 0);
  log("Add mapping for page %08lx at %08lx",
      (vaddr & PTE_MASK), (intptr_t)&PTE_OF(pmap, vaddr));

  /* invalidate corresponding entry in tlb */
  tlb_invalidate(PTE_VPN2(vaddr) | PTE_ASID(pmap->asid));

  /* TODO: what about caches? */
}

/* TODO: implement */
void pmap_clear_pte(pmap_t *pmap, vm_addr_t vaddr);

void pmap_map(pmap_t *pmap, vm_addr_t vaddr, pm_addr_t paddr, size_t npages,
              vm_prot_t prot) {
  assert(is_aligned(vaddr, PAGESIZE));
  assert(is_aligned(paddr, PAGESIZE));
  assert(vaddr >= pmap->start);
  assert(vaddr + npages * PAGESIZE <= pmap->end);

  for (size_t i = 0; i < npages; i++)
    pmap_set_pte(pmap, vaddr + i * PAGESIZE, paddr + i * PAGESIZE, prot);
}

/* TODO: implement */
void pmap_unmap(pmap_t *pmap, vm_addr_t vaddr, size_t npages);

/* TODO: this implementation is broken */
void pmap_protect(pmap_t *pmap, vm_addr_t vaddr, size_t npages,
                  vm_prot_t prot) {
  assert(vaddr >= pmap->start);
  assert(vaddr + npages * PAGESIZE <= pmap->end);

  for (size_t i = 0; i < npages; i++)
    /* Set 0xffffffff here, because there is no page with this
     * physical addres, so this is going to cause bus exception,
     * when trying to access this range */
    pmap_set_pte(pmap, vaddr + i * PAGESIZE, 0xfffffff, prot);
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

#define PDE_ID_FROM_PTE_ADDR(x) (((x) & 0x003ff000) >> 12)

__attribute__((interrupt)) 
void tlb_exception_handler() {
  int code = (mips32_get_c0(C0_CAUSE) & CR_X_MASK) >> CR_X_SHIFT;
  vm_addr_t vaddr = mips32_get_c0(C0_BADVADDR);

  tlb_print();

  kprintf("[tlb] %s at %p, caused by reference to %p!\n",
          exceptions[code], (void *)mips32_get_c0(C0_EPC), (void *)vaddr);

  intptr_t context = mips32_get_c0(C0_CONTEXT);
  intptr_t ptebase = (int)(context & PTEBASE_MASK) >> 1;
  intptr_t badvpn2 = (context & BADVPN2_MASK) << BADVPN2_SHIFT;
  asid_t asid = mips32_get_c0(C0_ENTRYHI) & PTE_ASID_MASK;

  kprintf("[tlb] PTEBASE: %08lx BADVPN2: %08lx ASID: %02x\n",
          ptebase, badvpn2, asid);

  /* If the fault was in virtual pt range it means it's time to refill */
  if (PTE_BASE <= vaddr && vaddr < PTE_BASE + PTE_SIZE) {
    uint32_t index = PDE_ID_FROM_PTE_ADDR(vaddr);
    kprintf("[tlb] fill TLB with page at %p\n", (void *)(vaddr & PTE_MASK));

    index &= ~1;

    bool user_addr = index < (PDE_ENTRIES / 2);
    pmap_t *pmap = get_active_pmap(user_addr ? PMAP_USER : PMAP_KERNEL);
    if (!pmap)
      panic("No user-space page table!");
    if (!(pmap->pde[index] & PTE_VALID))
      panic("Unmapped page table for address %08lx!", vaddr);

    vm_addr_t pte_start = PTE_BASE + index * PAGESIZE;
    tlbhi_t entryhi = pte_start | pmap->asid;
    tlblo_t entrylo0 = pmap->pde[index];
    tlblo_t entrylo1 = pmap->pde[index + 1];
    log("%08x %08x %08x", entryhi, entrylo0, entrylo1);
    tlb_overwrite_random(entryhi, entrylo0, entrylo1);
  } else {
    vm_map_t *map =
      get_active_vm_map(vaddr < 0x80000000 ? PMAP_USER : PMAP_KERNEL);
    if (!map)
      panic("No virtual address space defined for %08lx!", vaddr);
    vm_page_fault(map, vaddr, code == EXC_TLBL ? VM_PROT_READ : VM_PROT_WRITE);
  }
}

#ifdef _KERNELSPACE

#if 0
/* BUG: This should work! */
#define BASEADDR 0xc0400000
#else
#define BASEADDR 0xc0800000
#endif

static pmap_t kernel_pmap;

int main() {
  pmap_setup(&kernel_pmap, PMAP_KERNEL, 0);
  active_pmap[PMAP_KERNEL] = &kernel_pmap;

  pmap_t *pmap = get_active_pmap(PMAP_KERNEL);

  vm_page_t *pg1 = pm_alloc(4);
  vaddr_t vaddr = BASEADDR + PAGESIZE * 10;
  pmap_map(pmap, vaddr, pg1->paddr, pg1->size, VM_PROT_READ|VM_PROT_WRITE);

  {
    log("TLB before:");
    tlb_print();

    int *x = (int *)vaddr;
    for (int i = 0; i < 1024 * pg1->size; i++)
      *(x + i) = i;
    for (int i = 0; i < 1024 * pg1->size; i++)
      assert(*(x + i) == i);

    log("TLB after:");
    tlb_print();
  }

  vm_page_t *pg2 = pm_alloc(1);
  vaddr = BASEADDR + PAGESIZE * 2000;
  pmap_map(pmap, vaddr, pg2->paddr, pg2->size, VM_PROT_READ|VM_PROT_WRITE);

  {
    log("TLB before:");
    tlb_print();

    int *x = (int *)vaddr;
    for (int i = 0; i < 1024 * pg2->size; i++)
      *(x + i) = i;
    for (int i = 0; i < 1024 * pg2->size; i++)
      assert(*(x + i) == i);

    log("TLB after:");
    tlb_print();
  }

  pm_free(pg1);
  pm_free(pg2);

  pmap_reset(pmap);
  kprintf("Tests passed\n");
  return 0;
}
#endif /* _KERNELSPACE */
