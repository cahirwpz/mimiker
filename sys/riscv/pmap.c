#define KL_LOG KL_PMAP
#include <sys/kenv.h>
#include <sys/klog.h>
#include <sys/pmap.h>
#include <sys/_pmap.h>
#include <riscv/cpufunc.h>

/*
 * Page directory.
 */

pde_t pde_make(int lvl, paddr_t pa) {
  panic("Not implemented!");
}

void broadcast_kernel_top_pde(vaddr_t va, pde_t pde) {
  panic("Not implemented!");
}

/*
 * Page table.
 */

pte_t pte_make(paddr_t pa, vm_prot_t prot, unsigned flags) {
  panic("Not implemented!");
}

inline pte_t pte_protect(pte_t pte, vm_prot_t prot) {
  panic("Not implemented!");
}

/*
 * Physical map management.
 */

void pmap_md_setup(pmap_t *pmap) {
  panic("Not implemented!");
}

void pmap_md_activate(pmap_t *umap) {
  panic("Not implemented!");
}

void pmap_md_delete(pmap_t *pmap) {
  panic("Not implemented!");
}

void pmap_md_bootstrap(pde_t *pd) {
  dmap_paddr_base = kenv_get_ulong("mem_start");
  dmap_paddr_end = kenv_get_ulong("mem_end");
  size_t dmap_size = dmap_paddr_end - dmap_paddr_base;

  /* Assume physical memory starts at the beginning of L0 region. */
  assert(is_aligned(dmap_paddr_base, L0_SIZE));

  /* We must have enough virtual addresses. */
  assert(dmap_size <= DMAP_MAX_SIZE);

  /* For RV32 we assume 32-bit physical address space. */
  assert(dmap_paddr_base < dmap_paddr_end);

  klog("Physical memory range: %p - %p", dmap_paddr_base, dmap_paddr_end - 1);

  klog("dmap range: %p - %p", DMAP_BASE, DMAP_BASE + dmap_size - 1);

  /* Build direct map using superpages. */
  size_t idx = L0_INDEX(DMAP_BASE);
  for (paddr_t pa = dmap_paddr_base; pa < dmap_paddr_end; pa += L0_SIZE, idx++)
    pd[idx] = PA_TO_PTE(pa) | PTE_KERN;

  __sfence_vma();
}

/*
 * Direct map.
 */

void *phys_to_dmap(paddr_t addr) {
  assert((addr >= dmap_paddr_base) && (addr < dmap_paddr_end));
  return (void *)(addr - dmap_paddr_base) + DMAP_BASE;
}
