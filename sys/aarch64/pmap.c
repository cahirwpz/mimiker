#define KL_LOG KL_PMAP
#include <sys/klog.h>
#include <sys/pmap.h>
#include <sys/_pmap.h>
#include <aarch64/armreg.h>

/*
 * This table describes which access bits need to be set in page table entry
 * for successful memory translation by MMU. Other configurations causes memory
 * fault - see aarch64/trap.c.
 *
 * +--------------+----+------+----+----+
 * |    access    | AF | USER | RO | XN |
 * +==============+====+======+====+====+
 * | user read    | 1  | 1    | *  | *  |
 * +--------------+----+------+----+----+
 * | user write   | 1  | 1    | 0  | *  |
 * +--------------+----+------+----+----+
 * | user exec    | 1  | 1    | *  | 0  |
 * +--------------+----+------+----+----+
 * | kernel read  | 1  | *    | *  | *  |
 * +--------------+----+------+----+----+
 * | kernel write | 1  | *    | 0  | *  |
 * +--------------+----+------+----+----+
 * | kernel exec  | 1  | *    | *  | 0  |
 * +--------------+----+------+----+----+
 */

static const pte_t pte_common = L3_PAGE | ATTR_SH_IS;
static const pte_t pte_noexec = ATTR_XN | ATTR_SW_NOEXEC;

static const pte_t vm_prot_map[] = {
  [VM_PROT_NONE] = pte_noexec | pte_common,
  [VM_PROT_READ] =
    ATTR_AP_RO | ATTR_SW_READ | ATTR_AF | pte_noexec | pte_common,
  [VM_PROT_WRITE] =
    ATTR_AP_RW | ATTR_SW_WRITE | ATTR_AF | pte_noexec | pte_common,
  [VM_PROT_READ | VM_PROT_WRITE] = ATTR_AP_RW | ATTR_SW_READ | ATTR_SW_WRITE |
                                   ATTR_AF | pte_noexec | pte_common,
  [VM_PROT_EXEC] = ATTR_AF | pte_common,
  [VM_PROT_READ | VM_PROT_EXEC] =
    ATTR_AP_RO | ATTR_SW_READ | ATTR_AF | pte_common,
  [VM_PROT_WRITE | VM_PROT_EXEC] =
    ATTR_AP_RW | ATTR_SW_WRITE | ATTR_AF | pte_common,
  [VM_PROT_READ | VM_PROT_WRITE | VM_PROT_EXEC] =
    ATTR_AP_RW | ATTR_SW_READ | ATTR_SW_WRITE | ATTR_AF | pte_common,
};

static pde_t pd_lvl_masks[] = {
  [0] = L0_TABLE,
  [1] = L1_TABLE,
  [2] = L2_TABLE,
};

/*
 * Translation structure.
 */

inline size_t l0_index(vaddr_t va) {
  return L0_INDEX(va);
}

inline size_t l1_index(vaddr_t va) {
  return L1_INDEX(va);
}

inline size_t l2_index(vaddr_t va) {
  return L2_INDEX(va);
}

inline size_t l3_index(vaddr_t va) {
  return L3_INDEX(va);
}

/*
 * Page directory.
 */

inline bool pde_valid_p(pde_t pde) {
  return PTE_FRAME_ADDR(pde) != 0UL;
}

inline pde_t pde_make(paddr_t pa, unsigned lvl) {
  assert(lvl < ADDR_TRANSLATION_DEPTH - 1);
  return pa | pd_lvl_masks[lvl];
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
  return PTE_FRAME_ADDR(pte) != 0UL;
}

inline bool pte_readable(pte_t pte) {
  return pte & ATTR_SW_READ;
}

inline bool pte_writable(pte_t pte) {
  return pte & ATTR_SW_WRITE;
}

inline bool pte_executable(pte_t pte) {
  return !(pte & ATTR_SW_NOEXEC);
}

pte_t pte_make(paddr_t pa, vm_prot_t prot, unsigned flags, bool kernel) {
  pte_t pte = pa | vm_prot_map[prot];
  if (!kernel) {
    pte |= ATTR_AP_USER;
    pte &= ~ATTR_AF;
  }
  unsigned cacheflags = flags & PMAP_CACHE_MASK;
  if (cacheflags == PMAP_NOCACHE)
    return pte | ATTR_IDX(ATTR_NORMAL_MEM_NC);
  if (cacheflags == PMAP_WRITE_THROUGH)
    return pte | ATTR_IDX(ATTR_NORMAL_MEM_WT);
  return pte | ATTR_IDX(ATTR_NORMAL_MEM_WB);
}

inline pte_t pte_protect(pte_t pte, vm_prot_t prot) {
  return vm_prot_map[prot] | (pte & (~ATTR_AP_MASK & ~ATTR_XN));
}

inline paddr_t pte2pa(pte_t pte) {
  return PTE_FRAME_ADDR(pte);
}

/*
 * Physical map management.
 */

void pmap_md_activate(pmap_t *umap) {
  uint64_t tcr = READ_SPECIALREG(TCR_EL1);

  if (!umap) {
    WRITE_SPECIALREG(TCR_EL1, tcr | TCR_EPD0);
  } else {
    uint64_t ttbr0 = ((uint64_t)umap->asid << ASID_SHIFT) | umap->pde;
    WRITE_SPECIALREG(TTBR0_EL1, ttbr0);
    WRITE_SPECIALREG(TCR_EL1, tcr & ~TCR_EPD0);
  }
}

void pmap_md_setup(pmap_t *pmap __unused) {
  /* Nothing to be done here. */
}

void pmap_md_delete(pmap_t *pmap __unused) {
  /* Nothing to be done here. */
}

/*
 * Bootstrap.
 */

void pmap_bootstrap(paddr_t pd_pa) {
  dmap_paddr_base = 0;
  dmap_paddr_end = DMAP_SIZE;
  kernel_pde = pd_pa;
}
