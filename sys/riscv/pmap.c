#define KL_LOG KL_PMAP
#include <sys/kenv.h>
#include <sys/klog.h>
#include <sys/pmap.h>
#include <sys/_pmap.h>
#include <riscv/cpufunc.h>

/*
 * Translation structure.
 */

inline size_t l0_index(vaddr_t va) {
  panic("Not implemented!");
}

inline size_t l1_index(vaddr_t va) {
  panic("Not implemented!");
}

/*
 * Page directory.
 */

inline bool pde_valid_p(pde_t pde) {
  panic("Not implemented!");
}

inline pde_t pde_make(paddr_t pa, unsigned lvl) {
  panic("Not implemented!");
}

inline paddr_t pde2pa(pde_t pde) {
  panic("Not implemented!");
}

void kernel_pd_change_notif(pmap_t *pmap, vaddr_t va, pde_t pde) {
  panic("Not implemented!");
}

/*
 * Page table.
 */

inline bool pte_valid_p(pte_t pte) {
  panic("Not implemented!");
}

inline bool pte_readable(pte_t pte) {
  panic("Not implemented!");
}

inline bool pte_writable(pte_t pte) {
  panic("Not implemented!");
}

inline bool pte_executable(pte_t pte) {
  panic("Not implemented!");
}

pte_t pte_make(paddr_t pa, vm_prot_t prot, unsigned flags, bool kernel) {
  panic("Not implemented!");
}

inline pte_t pte_protect(pte_t pte, vm_prot_t prot) {
  panic("Not implemented!");
}

inline paddr_t pte2pa(pte_t pte) {
  panic("Not implemented!");
}

/*
 * Physical map management.
 */

void pmap_md_activate(pmap_t *umap) {
  panic("Not implemented!");
}

void pmap_md_setup(pmap_t *pmap) {
  panic("Not implemented!");
}

void pmap_md_delete(pmap_t *pmap) {
  panic("Not implemented!");
}

/*
 * Bootstrap.
 */

void pmap_bootstrap(paddr_t pd_pa, vaddr_t pd_va) {
  dmap_paddr_base = kenv_get_ulong("mem_start");
  dmap_paddr_end = kenv_get_ulong("mem_end");
  size_t dmap_size = dmap_paddr_end - dmap_paddr_base;
  kernel_pde = pd_pa;

  /* Assume physical memory starts at the beginning of L0 region. */
  assert(is_aligned(dmap_paddr_base, L0_SIZE));

  /* We must have enough virtual addresses. */
  assert(dmap_size <= DMAP_MAX_SIZE);

  /* For RV32 we assume 32-bit physical address space. */
  assert(dmap_paddr_base < dmap_paddr_end);

  klog("Physical memory range: %p - %p", dmap_paddr_base, dmap_paddr_end - 1);

  klog("dmap range: %p - %p", DMAP_BASE, DMAP_BASE + dmap_size - 1);

  /* Build direct map using superpages. */
  pde_t *pde = (void *)pd_va;
  size_t idx = L0_INDEX(DMAP_BASE);
  for (paddr_t pa = dmap_paddr_base; pa < dmap_paddr_end; pa += L0_SIZE, idx++)
    pde[idx] = PA_TO_PTE(pa) | PTE_KERN;

  __sfence_vma();
}
