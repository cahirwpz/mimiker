#define KL_LOG KL_PMAP
#include <sys/kenv.h>
#include <sys/klog.h>
#include <sys/libkern.h>
#include <sys/pmap.h>
#include <sys/_pmap.h>
#include <riscv/cpufunc.h>

/*
 * The following table describes which bits need to be set in page table
 * entry for successful memory translation by MMU. Other configurations cause
 * memory fault - see `riscv/trap.c`.
 *
 * +--------------+---+---+------+---+---+---+---+
 * |    access    | D | A | USER | X | W | R | V |
 * +==============+===+===+======+===+===+===+===+
 * | user read    | * | 1 | 1    | * | * | 1 | 1 |
 * +--------------+---+---+------+---+---+---+---+
 * | user write   | 1 | 1 | 1    | * | 1 | 1 | 1 |
 * +--------------+---+---+------+---+---+---+---+
 * | user exec    | * | 1 | 1    | 1 | * | * | 1 |
 * +--------------+---+---+------+---+---+---+---+
 * | kernel read  | * | 1 | 0    | * | * | 1 | 1 |
 * +--------------+---+---+------+---+---+---+---+
 * | kernel write | 1 | 1 | 0    | * | 1 | 1 | 1 |
 * +--------------+---+---+------+---+---+---+---+
 * | kernel exec  | * | 1 | 0    | 1 | * | * | 1 |
 * +--------------+---+---+------+---+---+---+---+
 *
 * The dirty (D) and accessed (A) bits may be managed automaticaly
 * by hardware. In such case, setting of these bits is imperceptible from the
 * perspective of the software. To be compliant with the other ports,
 * we assume these bits to be unsupported and emulate them in software.
 */

static const pte_t pte_common = PTE_A | PTE_V;

static const pte_t vm_prot_map[] = {
  [VM_PROT_READ] = PTE_SW_READ | PTE_R | pte_common,
  [VM_PROT_WRITE] = PTE_SW_WRITE | PTE_D | PTE_W | PTE_R | pte_common,
  [VM_PROT_READ | VM_PROT_WRITE] =
    PTE_SW_WRITE | PTE_SW_READ | PTE_D | PTE_W | PTE_R | pte_common,
  [VM_PROT_EXEC] = pte_common,
  [VM_PROT_READ | VM_PROT_EXEC] = PTE_SW_READ | PTE_X | PTE_R | pte_common,
  [VM_PROT_WRITE | VM_PROT_EXEC] =
    PTE_SW_WRITE | PTE_D | PTE_X | PTE_W | PTE_R | pte_common,
  [VM_PROT_READ | VM_PROT_WRITE | VM_PROT_EXEC] =
    PTE_SW_WRITE | PTE_SW_READ | PTE_D | PTE_X | PTE_W | PTE_R | pte_common,
};

/*
 * Page directory.
 */

pde_t pde_make(unsigned lvl, paddr_t pa) {
  return PA_TO_PTE(pa) | PTE_V;
}

/*
 * Page table.
 */

pte_t pte_make(paddr_t pa, vm_prot_t prot, unsigned flags, bool kernel) {
  pte_t pte = PA_TO_PTE(pa) | vm_prot_map[prot];

  const pte_t mask_on = (kernel) ? PTE_G : PTE_U;
  pte |= mask_on;

  const pte_t mask_off = (kernel) ? 0 : PTE_D | PTE_A | PTE_W | PTE_V;
  pte &= ~mask_off;

  /* TODO(MichalBlk): if the target board supports PMA setting,
   * set the attributes according to cache flags passed in `flags`. */

  return pte;
}

inline pte_t pte_protect(pte_t pte, vm_prot_t prot) {
  return (pte & ~PTE_PROT_MASK) | vm_prot_map[prot];
}

/*
 * Physical map management.
 */

void pmap_md_activate(pmap_t *umap) {
  __set_satp(umap->md.satp);
  __sfence_vma();
}

void pmap_md_setup(pmap_t *pmap) {
  pmap->md.satp = SATP_MODE_SV32 | ((paddr_t)pmap->asid << SATP_ASID_S) |
                  (pmap->pde >> PAGE_SHIFT);
}

void pmap_md_delete(pmap_t *pmap) {
  /* Nothing to be done here. */
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
