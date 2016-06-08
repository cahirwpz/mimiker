#include <pmap.h>
#include <malloc.h>
#include <libkern.h>
#include <mips/cpu.h>
#include <tlb.h>

#define PTE_INDEX(x)  (((x) & 0xfffff000) >> 12)
#define PDE_INDEX(x) (((x) & 0xffc00000) >> 22)

static pmap_t *active_pmap = NULL;

void pmap_init(pmap_t *pmap) {
  pmap->pte = (pte_t *)PTE_BASE;
  pmap->pde_page = vm_phys_alloc(0);
  pmap->pde = (pte_t *)pmap->pde_page->virt_addr;
  TAILQ_INIT(&pmap->pte_pages);
}

void pmap_delete(pmap_t *pmap) {
  while (!TAILQ_EMPTY(&pmap->pte_pages)) {
    vm_page_t *pg = TAILQ_FIRST(&pmap->pte_pages);
    TAILQ_REMOVE(&pmap->pte_pages, pg, pt.list);
    vm_phys_free(pg);
  }
  vm_phys_free(pmap->pde_page);
}

static void pde_map(pmap_t *pmap, vaddr_t vaddr) {
  uint32_t pde_index = PDE_INDEX(vaddr);
  if ( (pmap->pde[pde_index] & V_MASK) ==
       0) { /* part of page table isn't located in memory */
    vm_page_t *pg = vm_phys_alloc(0);
    TAILQ_INSERT_TAIL(&pmap->pte_pages, pg, pt.list);

    ENTRYLO_SET_PADDR(pmap->pde[pde_index], pg->phys_addr);
    ENTRYLO_SET_V(pmap->pde[pde_index], 1);
    ENTRYLO_SET_D(pmap->pde[pde_index], 1);
  }

  /* make sure proper address is in tlb */
  pde_index &= ~1;
  tlbhi_t entryhi = (PTE_BASE + pde_index * PAGESIZE) | pmap->asid;
  tlblo_t entrylo0 = pmap->pde[pde_index];
  tlblo_t entrylo1 = pmap->pde[pde_index + 1];
  tlb_overwrite_random(entryhi, entrylo0, entrylo1);
}

static void pt_map(pmap_t *pmap, vm_addr_t vaddr, vm_paddr_t paddr,
                   uint32_t flags) {
  pde_map(pmap, vaddr);
  pte_t entry = flags;
  uint32_t pt_index = PTE_INDEX(vaddr);
  ENTRYLO_SET_PADDR(entry , paddr);
  pmap->pte[pt_index] = entry;

  /* invalidate proper entry in tlb */
  tlbhi_t entryhi = 0;
  ENTRYHI_SET_VADDR(entryhi, vaddr);
  ENTRYHI_SET_ASID(entryhi, pmap->asid);
  tlb_invalidate(entryhi);
}

void pmap_map(pmap_t *pmap, vm_addr_t vaddr, vm_paddr_t paddr, size_t npages,
              uint32_t flags) {
  for (size_t i = 0; i < npages; i++)
    pt_map(pmap, vaddr + i * PAGESIZE, paddr + i * PAGESIZE, flags);
}

void pmap_unmap(pmap_t *pmap, vm_addr_t vaddr, size_t npages) {
  panic("unimplemented");
}

void set_active_pmap(pmap_t *pmap) {
  active_pmap = pmap;
  mips32_set_c0(C0_ENTRYHI, pmap->asid);
}

pmap_t *get_active_pmap() {
  return active_pmap;
}

#ifdef _KERNELSPACE
int main() { /* Simple test */
  pmap_t pmap;
  pmap_init(&pmap);
  pmap.asid = 10;

  set_active_pmap(&pmap);
  vm_page_t *pg1 = vm_phys_alloc(2);

  vaddr_t ex_addr = PAGESIZE * 10;
  pmap_map(&pmap, ex_addr, pg1->phys_addr, 1 << pg1->order,
           PMAP_VALID | PMAP_DIRTY);

  int *x = (int *) ex_addr;
  for (int i = 0; i < 1024 * (1 << pg1->order); i++)
    *(x + i) = i;
  for (int i = 0; i < 1024 * (1 << pg1->order); i++)
    assert(*(x + i) == i);

  vm_page_t *pg2 = vm_phys_alloc(0);

  ex_addr = PAGESIZE * 2000;
  pmap_map(&pmap, ex_addr, pg2->phys_addr, 1 << pg2->order,
           PMAP_VALID | PMAP_DIRTY);

  x = (int *) ex_addr;
  for (int i = 0; i < 1024 * (1 << pg2->order); i++)
    *(x + i) = i;
  for (int i = 0; i < 1024 * (1 << pg2->order); i++)
    assert(*(x + i) == i);

  vm_phys_free(pg1);
  vm_phys_free(pg2);

  pmap_delete(&pmap);
  return 0;
}
#endif /* _KERNELSPACE */

