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
 * The list of all the user pmaps.
 * Order of acquiring locks:
 *  - `pmap_kernel()->mtx`
 *  - `user_pmaps_lock`
 */
static MTX_DEFINE(user_pmaps_lock, 0);
static LIST_HEAD(, pmap) user_pmaps = LIST_HEAD_INITIALIZER(user_pmaps);

/*
 * Page directory.
 */

pde_t pde_make(int lvl, paddr_t pa) {
  return PA_TO_PTE(pa) | PTE_V;
}

/*
 * Page table.
 */

pte_t pte_make(paddr_t pa, vm_prot_t prot, unsigned flags) {
  pte_t pte = PA_TO_PTE(pa) | vm_prot_map[prot];
  bool kernel = flags & _PMAP_KERNEL;

  const pte_t mask_on = kernel ? PTE_G : PTE_U;
  pte |= mask_on;

  const pte_t mask_off = kernel ? 0 : PTE_D | PTE_A | PTE_W | PTE_V;
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
  pmap_t *kmap = pmap_kernel();
  assert(kmap->md.generation);

  if (umap->md.generation < kmap->md.generation) {
    /* Update kernel page directory entries within the pmap. */
    size_t halfpage = PAGESIZE / 2;
    WITH_MTX_LOCK (&kmap->mtx) {
      memcpy(phys_to_dmap(umap->pde) + halfpage,
             phys_to_dmap(kmap->pde) + halfpage, halfpage);
    }
    umap->md.generation = kmap->md.generation;
  }

  __set_satp(umap->md.satp);
  __sfence_vma();
}

void pmap_md_setup(pmap_t *pmap) {
  pmap->md.satp = SATP_MODE_SV32 | ((paddr_t)pmap->asid << SATP_ASID_S) |
                  (pmap->pde >> PAGE_SHIFT);
  pmap->md.generation = (pmap == pmap_kernel());

  WITH_MTX_LOCK (&user_pmaps_lock)
    LIST_INSERT_HEAD(&user_pmaps, pmap, md.pmap_link);
}

void pmap_md_delete(pmap_t *pmap) {
  WITH_MTX_LOCK (&user_pmaps_lock)
    LIST_REMOVE(pmap, md.pmap_link);
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

void pmap_md_growkernel(vaddr_t maxkvaddr) {
  pmap_t *kmap = pmap_kernel();
  SCOPED_MTX_LOCK(&kmap->mtx);

  assert(kmap->md.generation);

  if (++kmap->md.generation == 0) {
    pmap_t *umap;
    WITH_MTX_LOCK (&user_pmaps_lock)
      LIST_FOREACH (umap, &user_pmaps, md.pmap_link)
        umap->md.generation = 0;
    kmap->md.generation = 1;
  }
}

/*
 * Direct map.
 */

void *phys_to_dmap(paddr_t addr) {
  assert((addr >= dmap_paddr_base) && (addr < dmap_paddr_end));
  return (void *)(addr - dmap_paddr_base) + DMAP_BASE;
}
