#define KL_LOG KL_PMAP
#include <sys/klog.h>
#include <sys/mimiker.h>
#include <aarch64/context.h>
#include <aarch64/pmap.h>
#include <aarch64/pte.h>
#include <sys/pmap.h>
#include <sys/vm.h>
#include <sys/spinlock.h>
#include <bitstring.h>
#include <sys/vm_physmem.h>
#include <sys/pool.h>

static POOL_DEFINE(P_PMAP, "pmap", sizeof(pmap_t));

#define ADDR_MASK 0x8ffffffff000
#define DMAP_BASE 0xffffff8000000000 /* last 512GB */
#define PHYS_TO_DMAP(x) ((intptr_t)(x) + DMAP_BASE)

/* Kernel page directory entries. */
alignas(PAGESIZE) pde_t _kernel_pmap_pde[PD_ENTRIES];

static pmap_t kernel_pmap;

#define MAX_ASID 0xFF
static bitstr_t asid_used[bitstr_size(MAX_ASID)] = {0};
static spin_t *asid_lock = &SPIN_INITIALIZER(0);

static asid_t alloc_asid(void) {
  int free = 0;
  WITH_SPIN_LOCK (asid_lock) {
    bit_ffc(asid_used, MAX_ASID, &free);
    if (free < 0)
      panic("Out of asids!");
    bit_set(asid_used, free);
  }
  klog("alloc_asid() = %d", free);
  return free;
}

static pde_t *pmap_l0(pmap_t *pmap, vaddr_t va) {
  return &pmap->pde[L0_INDEX(va)];
}

static pde_t *pmap_l1(pmap_t *pmap, vaddr_t va) {
  pde_t l1 = *pmap_l0(pmap, va) & ADDR_MASK;
  pde_t *dmap_l1 = (pde_t *)PHYS_TO_DMAP(l1);
  return &dmap_l1[L1_INDEX(va)];
}

static pde_t *pmap_l2(pmap_t *pmap, vaddr_t va) {
  pde_t l2 = *pmap_l1(pmap, va) & ADDR_MASK;
  pde_t *dmap_l2 = (pde_t *)PHYS_TO_DMAP(l2);
  return &dmap_l2[L2_INDEX(va)];
}

static pte_t *pmap_l3(pmap_t *pmap, vaddr_t va) {
  pte_t l3 = *pmap_l2(pmap, va) & ADDR_MASK;
  pte_t *dmap_l3 = (pte_t *)PHYS_TO_DMAP(l3);
  return &dmap_l3[L3_INDEX(va)];
}

static pde_t *pmap_alloc_pde(pmap_t *pmap, vaddr_t vaddr, int level) {
  vm_page_t *pg = vm_page_alloc(1);
  pde_t *pde = (pde_t *)PHYS_TO_DMAP(pg->paddr);

  if (level == 3)
    TAILQ_INSERT_TAIL(&pmap->pte_pages, pg, pageq);

  klog("Page table level %d for 0x%016lx allocated at 0x%016lx", level, vaddr,
       pg->paddr);

  for (int i = 0; i < PD_ENTRIES; ++i)
    pde[i] = 0;

  return (pde_t *)pg->paddr;
}

inline bool pmap_contains_p(pmap_t *pmap, vaddr_t start, vaddr_t end) {
  return pmap_start(pmap) <= start && end <= pmap_end(pmap);
}

vaddr_t pmap_start(pmap_t *pmap) {
  panic("Not implemented!");
}

vaddr_t pmap_end(pmap_t *pmap) {
  panic("Not implemented!");
}

void pmap_reset(pmap_t *pmap) {
  panic("Not implemented!");
}

static void pmap_setup(pmap_t *pmap) {
  pmap->asid = alloc_asid();
  mtx_init(&pmap->mtx, 0);
  TAILQ_INIT(&pmap->pte_pages);
}

void init_pmap(void) {
  pmap_setup(&kernel_pmap);
  kernel_pmap.pde = _kernel_pmap_pde;
}

pmap_t *pmap_new(void) {
  pmap_t *pmap = pool_alloc(P_PMAP, M_ZERO);
  pmap_setup(pmap);
  return pmap;
}

void pmap_delete(pmap_t *pmap) {
  panic("Not implemented!");
}

static const pte_t pte_default =
  L3_PAGE | ATTR_AF | ATTR_SH(ATTR_SH_IS) | ATTR_IDX(ATTR_NORMAL_MEM_WB);

static const pte_t vm_prot_map[] = {
  [VM_PROT_NONE] = ATTR_XN | pte_default,
  [VM_PROT_READ] = ATTR_AP(ATTR_AP_RO) | ATTR_XN | pte_default,
  [VM_PROT_WRITE] = ATTR_AP(ATTR_AP_RW) | ATTR_XN | pte_default,
  [VM_PROT_READ | VM_PROT_WRITE] = ATTR_AP(ATTR_AP_RW) | ATTR_XN | pte_default,
  [VM_PROT_EXEC] = pte_default,
  [VM_PROT_READ | VM_PROT_EXEC] = ATTR_AP(ATTR_AP_RO) | pte_default,
  [VM_PROT_WRITE | VM_PROT_EXEC] = ATTR_AP(ATTR_AP_RW) | pte_default,
  [VM_PROT_READ | VM_PROT_WRITE | VM_PROT_EXEC] =
    ATTR_AP(ATTR_AP_RW) | pte_default,
};

void pmap_kenter(paddr_t va, paddr_t pa, vm_prot_t prot, unsigned flags) {
  pmap_t *pmap = pmap_kernel();

  pte_t pte = vm_prot_map[prot];

  WITH_MTX_LOCK (&pmap->mtx) {
    pde_t *l0 = pmap_l0(pmap, va);
    if (*l0 == 0)
      *l0 = (pde_t)pmap_alloc_pde(pmap, va, 1) | L0_TABLE;

    pde_t *l1 = pmap_l1(pmap, va);
    if (*l1 == 0)
      *l1 = (pde_t)pmap_alloc_pde(pmap, va, 2) | L1_TABLE;

    pde_t *l2 = pmap_l2(pmap, va);
    if (*l2 == 0)
      *l2 = (pde_t)pmap_alloc_pde(pmap, va, 3) | L2_TABLE;

    pte_t *l3 = pmap_l3(pmap, va);
    *l3 = pa | pte;
  }
}

void pmap_enter(pmap_t *pmap, vaddr_t va, vm_page_t *pg, vm_prot_t prot,
                unsigned flags) {
  panic("Not implemented!");
}

void pmap_kremove(vaddr_t start, vaddr_t end) {
  pmap_remove(pmap_kernel(), start, end);
}

void pmap_remove(pmap_t *pmap, vaddr_t start, vaddr_t end) {
  assert(page_aligned_p(start) && page_aligned_p(end) && start < end);
  assert(pmap_contains_p(pmap, start, end));

  klog("Remove page mapping for address range %p-%p", start, end);

  WITH_MTX_LOCK (&pmap->mtx) {
    for (vaddr_t va = start; va < end; va += PAGESIZE) {
      pte_t *l3 = pmap_l3(pmap, va);
      *l3 = 0;
    }
  }
}

void pmap_protect(pmap_t *pmap, vaddr_t start, vaddr_t end, vm_prot_t prot) {
  panic("Not implemented!");
}

bool pmap_extract(pmap_t *pmap, vaddr_t va, paddr_t *pap) {
  panic("Not implemented!");
}

void pmap_zero_page(vm_page_t *pg) {
  panic("Not implemented!");
}

void pmap_copy_page(vm_page_t *src, vm_page_t *dst) {
  panic("Not implemented!");
}

void pmap_activate(pmap_t *pmap) {
  if (pmap != pmap_kernel()) {
    panic("Not implemented!");
  }
}

pmap_t *pmap_kernel(void) {
  return &kernel_pmap;
}

pmap_t *pmap_user(void) {
  panic("Not implemented!");
}

pmap_t *pmap_lookup(vaddr_t addr) {
  panic("Not implemented!");
}

void tlb_exception_handler(ctx_t *ctx) {
  panic("Not implemented!");
}
