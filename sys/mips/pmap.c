#define KL_LOG KL_PMAP
#include <sys/klog.h>
#include <sys/pmap.h>
#include <sys/_pmap.h>
#include <mips/tlb.h>

#define PTE_FRAME_ADDR(pte) (PTE_PFN_OF(pte) * PAGESIZE)

static const pte_t vm_prot_map[] = {
  [VM_PROT_NONE] = PTE_SW_NOEXEC,
  [VM_PROT_READ] = PTE_VALID | PTE_SW_READ | PTE_SW_NOEXEC,
  [VM_PROT_WRITE] = PTE_VALID | PTE_DIRTY | PTE_SW_WRITE | PTE_SW_NOEXEC,
  [VM_PROT_READ | VM_PROT_WRITE] =
    PTE_VALID | PTE_DIRTY | PTE_SW_READ | PTE_SW_WRITE | PTE_SW_NOEXEC,
  [VM_PROT_EXEC] = PTE_VALID,
  [VM_PROT_READ | VM_PROT_EXEC] = PTE_VALID | PTE_SW_READ,
  [VM_PROT_WRITE | VM_PROT_EXEC] = PTE_VALID | PTE_DIRTY | PTE_SW_WRITE,
  [VM_PROT_READ | VM_PROT_WRITE | VM_PROT_EXEC] =
    PTE_VALID | PTE_DIRTY | PTE_SW_READ | PTE_SW_WRITE,
};

/*
 * Translation structure.
 */

inline size_t l0_index(vaddr_t va) {
  return PDE_INDEX(va);
}

inline size_t l1_index(vaddr_t va) {
  return PTE_INDEX(va);
}

/*
 * Page directory.
 */

inline bool pde_valid_p(pde_t pde) {
  return pde & PDE_VALID;
}

inline pde_t pde_make(paddr_t pa, unsigned lvl __unused) {
  return PTE_PFN(phys_to_dmap(pa)) | PTE_KERNEL;
}

inline paddr_t pde2pa(pde_t pde) {
  return PTE_FRAME_ADDR(pde);
}

void kernel_pd_change_notif(pmap_t *pmap, vaddr_t va, pde_t pde) {
  /* Nothing to be done here. */
}

/*
 * Page table.
 */

inline bool pte_valid_p(pte_t pte) {
  return PTE_FRAME_ADDR(pte) != 0;
}

inline bool pte_readable(pte_t pte) {
  return pte & PTE_SW_READ;
}

inline bool pte_writable(pte_t pte) {
  return pte & PTE_SW_WRITE;
}

inline bool pte_executable(pte_t pte) {
  return !(pte & PTE_SW_NOEXEC);
}

pte_t pte_make(paddr_t pa, vm_prot_t prot, unsigned flags, bool kernel) {
  pte_t pte = PTE_PFN(pa) | vm_prot_map[prot];
  if (kernel)
    pte |= PTE_GLOBAL;
  else
    pte &= ~(PTE_VALID | PTE_DIRTY);

  unsigned cacheflags = flags & PMAP_CACHE_MASK;
  if (cacheflags == PMAP_NOCACHE)
    pte |= PTE_CACHE_UNCACHED;
  else if (cacheflags == PMAP_WRITE_THROUGH)
    pte |= PTE_CACHE_WRITE_THROUGH;
  else
    pte |= PTE_CACHE_WRITE_BACK;

  return pte;
}

inline pte_t pte_protect(pte_t pte, vm_prot_t prot) {
  return (pte & ~PTE_PROT_MASK) | vm_prot_map[prot];
}

inline paddr_t pte2pa(pte_t pte) {
  return PTE_FRAME_ADDR(pte);
}

/*
 * Physical map management.
 */

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

void pmap_md_setup(pmap_t *pmap) {
  if (pmap == pmap_kernel())
    return;
  pmap->pde = (paddr_t)MIPS_PHYS_TO_KSEG0(pmap->pde);
}

void pmap_md_delete(pmap_t *pmap __unused) {
  /* Nothing to be done here. */
}
