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

/*
 * Since `pmap_md_update` and `pmap_md_update` are called (via `vm_map_switch`)
 * from `ctx_switch`, they may be executed in an interrupt context. Thus they
 * cannot take any sleep lock including default mutex.
 *
 * That's why we need to introduce spin lock for synchronization of level 0
 * page directory entries that belong to the kernel.
 */
static MTX_DEFINE(kmap_l0_lock, MTX_SPIN);
static paddr_t kmap_l0_pages[Ln_ENTRIES];

void pmap_md_update(pmap_t *umap) {
  if (umap == NULL)
    return;

  pmap_t *kmap = pmap_kernel();

  assert(umap != kmap);

  WITH_MTX_LOCK (&kmap_l0_lock) {
    if (umap->md.generation == kmap->md.generation)
      return;

    /* Synchronize kernel page directory entries between two pmaps. */
    size_t halfpage = PAGESIZE / 2;
    void *new_kpd = phys_to_dmap(kmap->pde) + halfpage;
    void *old_kpd = phys_to_dmap(umap->pde) + halfpage;
    memcpy(old_kpd, new_kpd, halfpage);

    __sfence_vma();

    umap->md.generation = kmap->md.generation;
  }
}

void pmap_md_growkernel(vaddr_t old_kva, vaddr_t new_kva) {
  /* Did we change top kernel PD? If so force updates to user pmaps! */
  if ((old_kva & ~L0_OFFSET) == (new_kva & ~L0_OFFSET))
    return;

  pmap_t *kmap = pmap_kernel();

  /* We're protected by unique `kernel_pmap.mtx` */
  assert(mtx_owned(&kmap->mtx));

  old_kva = roundup(old_kva, L0_SIZE);

  /* Cannot allocate memory under spin lock, so do it here. */
  for (vaddr_t va = old_kva; va <= new_kva; va += L0_SIZE)
    kmap_l0_pages[L0_INDEX(va)] = pde_alloc(kmap);

  WITH_MTX_LOCK (&kmap_l0_lock) {
    /* Fill level 0 page directory with new pages. */
    for (vaddr_t va = old_kva; va <= new_kva; va += L0_SIZE) {
      pde_t *pdep = pde_ptr(kmap->pde, 0, va);
      assert(!pde_valid_p(pdep));
      *pdep = pde_make(0, kmap_l0_pages[L0_INDEX(va)]);
    }

    kmap->md.generation++;
    assert(kmap->md.generation);
  }
}

void pmap_md_activate(pmap_t *umap) {
  pmap_t *kmap = pmap_kernel();
  paddr_t satp = umap ? umap->md.satp : kmap->md.satp;

  pmap_md_update(umap);

  __set_satp(satp);
  __sfence_vma();
}

void pmap_md_setup(pmap_t *pmap) {
#if __riscv_xlen == 64
  pmap->md.satp = SATP_MODE_SV39 | ((paddr_t)pmap->asid << SATP_ASID_S) |
                  (pmap->pde >> PAGE_SHIFT);
#else
  pmap->md.satp = SATP_MODE_SV32 | ((paddr_t)pmap->asid << SATP_ASID_S) |
                  (pmap->pde >> PAGE_SHIFT);
#endif
  pmap->md.generation = (pmap == pmap_kernel());
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
