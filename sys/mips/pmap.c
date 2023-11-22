#define KL_LOG KL_PMAP
#include <sys/klog.h>
#include <sys/pmap.h>
#include <sys/_pmap.h>
#include <mips/tlb.h>

static const pte_t vm_prot_map[] = {
  [VM_PROT_NONE] = PTE_SW_NOEXEC | PTE_RI | PTE_XI,
  [VM_PROT_READ] = PTE_VALID | PTE_SW_READ | PTE_SW_NOEXEC | PTE_XI,
  [VM_PROT_WRITE] =
    PTE_VALID | PTE_DIRTY | PTE_SW_WRITE | PTE_SW_NOEXEC | PTE_RI | PTE_XI,
  [VM_PROT_READ | VM_PROT_WRITE] =
    PTE_VALID | PTE_DIRTY | PTE_SW_READ | PTE_SW_WRITE | PTE_SW_NOEXEC | PTE_XI,
  [VM_PROT_EXEC] = PTE_VALID | PTE_RI,
  [VM_PROT_READ | VM_PROT_EXEC] = PTE_VALID | PTE_SW_READ,
  [VM_PROT_WRITE | VM_PROT_EXEC] =
    PTE_VALID | PTE_DIRTY | PTE_SW_WRITE | PTE_RI,
  [VM_PROT_READ | VM_PROT_WRITE | VM_PROT_EXEC] =
    PTE_VALID | PTE_DIRTY | PTE_SW_READ | PTE_SW_WRITE,
};

/*
 * Page directory.
 */

pde_t pde_make(int lvl, paddr_t pa) {
  pde_t *pde = (pde_t *)phys_to_dmap(pa);
  for (int i = 0; i < PT_ENTRIES; i++)
    pde[i] = PTE_GLOBAL;
  return PTE_PFN((paddr_t)pde) | PTE_KERNEL;
}

/*
 * Page table.
 */

pte_t pte_make(paddr_t pa, vm_prot_t prot, unsigned flags) {
  pte_t pte = PTE_PFN(pa) | vm_prot_map[prot];
  if (flags & _PMAP_KERNEL)
    pte |= PTE_GLOBAL;
  else
    pte &= ~(PTE_VALID | PTE_DIRTY);

  if (flags & PMAP_NOCACHE)
    pte |= PTE_CACHE_UNCACHED;
  else if (flags & PMAP_WRITE_THROUGH)
    pte |= PTE_CACHE_WRITE_THROUGH;
  else
    pte |= PTE_CACHE_WRITE_BACK;

  return pte;
}

pte_t pte_protect(pte_t pte, vm_prot_t prot) {
  return (pte & ~PTE_PROT_MASK) | vm_prot_map[prot];
}

/*
 * Physical map management.
 */

void pmap_md_setup(pmap_t *pmap) {
  if (pmap == pmap_kernel())
    return;
  pmap->pde = (paddr_t)MIPS_PHYS_TO_KSEG0(pmap->pde);
}

void pmap_md_activate(pmap_t *umap) {
  pmap_t *kmap = pmap_kernel();

  /* Both ENTRYLO0 and ENTRYLO1 must have G bit set for address translation
   * to skip ASID check. */
  tlbentry_t e = {.hi = PTE_VPN2(UPD_BASE),
                  .lo0 = PTE_GLOBAL,
                  .lo1 = PTE_PFN(MIPS_KSEG0_TO_PHYS(kmap->pde)) | PTE_KERNEL};

  if (umap)
    e.lo0 = PTE_PFN(MIPS_KSEG0_TO_PHYS(umap->pde)) | PTE_KERNEL;

  tlb_write(0, &e);

  /* Set ASID for current process */
  mips32_setentryhi(umap ? umap->asid : 0);
}

void pmap_md_bootstrap(pde_t *pd __unused) {
  dmap_paddr_base = 0;
  dmap_paddr_end = MIPS_KSEG1_START - MIPS_KSEG0_START;
}

/*
 * Direct map.
 */

__no_profile void *phys_to_dmap(paddr_t addr) {
  if (addr >= DMAP_BASE)
    return (void *)addr;
  return (void *)(addr - dmap_paddr_base) + DMAP_BASE;
}
