#define KL_LOG KL_PMAP
#include <klog.h>
#include <pool.h>
#include <physmem.h>
#include <mips/exc.h>
#include <mips/mips.h>
#include <mips/tlb.h>
#include <mips/pmap.h>
#include <pcpu.h>
#include <pmap.h>
#include <vm_map.h>
#include <thread.h>
#include <ktest.h>
#include <signal.h>
#include <spinlock.h>
#include <mutex.h>
#include <sched.h>
#include <interrupt.h>
#include <sysinit.h>

static POOL_DEFINE(P_PMAP, "pmap", sizeof(pmap_t));

#define PTE_INDEX(x) (((x)&PTE_MASK) >> PTE_SHIFT)
#define PDE_INDEX(x) (((x)&PDE_MASK) >> PDE_SHIFT)

#define PTE_OF(pmap, addr) ((pmap)->pte[PTE_INDEX(addr)])
#define PDE_OF(pmap, addr) ((pmap)->pde[PDE_INDEX(addr)])
#define PTF_ADDR_OF(vaddr) (PT_BASE + PDE_INDEX(vaddr) * PTF_SIZE)

#define PTE_KERNEL (PTE_VALID | PTE_DIRTY | PTE_GLOBAL)

static bool is_valid(pte_t pte) {
  return pte & PTE_VALID;
}

static bool in_user_space(vaddr_t addr) {
  return addr < PMAP_USER_END;
}

static bool in_kernel_space(vaddr_t addr) {
  return (addr >= PMAP_KERNEL_BEGIN && addr < PMAP_KERNEL_END);
}

static pmap_t kernel_pmap;
static bitstr_t asid_used[bitstr_size(MAX_ASID)] = {0};
static spinlock_t *asid_lock = &SPINLOCK_INITIALIZER();

static asid_t alloc_asid(void) {
  int free;
  WITH_SPINLOCK(asid_lock) {
    bit_ffc(asid_used, MAX_ASID, &free);
    if (free < 0)
      panic("Out of asids!");
    bit_set(asid_used, free);
  }
  klog("alloc_asid() = %d", free);
  return free;
}

static void free_asid(asid_t asid) {
  klog("free_asid(%d)", asid);
  SCOPED_SPINLOCK(asid_lock);
  bit_clear(asid_used, (unsigned)asid);
  tlb_invalidate_asid(asid);
}

static void update_wired_pde(pmap_t *umap) {
  pmap_t *kmap = get_kernel_pmap();

  /* Both ENTRYLO0 and ENTRYLO1 must have G bit set for address translation
   * to skip ASID check. */
  tlbentry_t e = {.hi = PTE_VPN2(PD_BASE),
                  .lo0 = PTE_GLOBAL,
                  .lo1 = PTE_PFN(kmap->pde_page->paddr) | PTE_KERNEL};

  if (umap)
    e.lo0 = PTE_PFN(umap->pde_page->paddr) | PTE_KERNEL;

  tlb_write(0, &e);
}

static void pmap_setup(pmap_t *pmap, vaddr_t start, vaddr_t end) {
  bool user_pde = in_user_space(start);

  /* Place user & kernel PDEs after virtualized page table. */
  vm_page_t *pde_page = pm_alloc(1);
  vaddr_t pde_addr = PD_BASE + (user_pde ? 0 : PD_SIZE);

  pmap->pte = (pte_t *)PT_BASE;
  pmap->pde_page = pde_page;
  pmap->pde = (pte_t *)pde_addr;
  pmap->start = start;
  pmap->end = end;
  pmap->asid = alloc_asid();
  mtx_init(&pmap->mtx, MTX_DEF);
  klog("Page directory table allocated at %p", (vaddr_t)pmap->pde);
  TAILQ_INIT(&pmap->pte_pages);

  pmap_t *old_user_pmap = get_user_pmap();

  /*
   * No preemption here! Consider a case where this thread gets preempted and
   * other thread calls pmap_setup as well.
   */
  WITH_NO_PREEMPTION {
    /*
     * XXX: To initialize user pmap its PD is temporarily mapped in place
     * of current PD. This way we can access the PD using virtual addresses.
     * This is a workaround and probably can be done better.
     */
    update_wired_pde(user_pde ? pmap : NULL);

    for (int i = 0; i < PD_ENTRIES; i++)
      pmap->pde[i] =
        in_kernel_space(i * PTF_ENTRIES * PAGESIZE) ? PTE_GLOBAL : 0;

    update_wired_pde(old_user_pmap);
  }
}

/* TODO: remove all mappings from TLB, evict related cache lines */
void pmap_reset(pmap_t *pmap) {
  while (!TAILQ_EMPTY(&pmap->pte_pages)) {
    vm_page_t *pg = TAILQ_FIRST(&pmap->pte_pages);
    TAILQ_REMOVE(&pmap->pte_pages, pg, pageq);
    pm_free(pg);
  }
  pm_free(pmap->pde_page);
  free_asid(pmap->asid);
}

void pmap_init(void) {
  pmap_setup(&kernel_pmap, PMAP_KERNEL_BEGIN + PT_SIZE + PD_SIZE * 2,
             PMAP_KERNEL_END);
}

pmap_t *pmap_new(void) {
  pmap_t *pmap = pool_alloc(P_PMAP, PF_ZERO);
  pmap_setup(pmap, PMAP_USER_BEGIN, PMAP_USER_END);
  return pmap;
}

void pmap_delete(pmap_t *pmap) {
  pmap_reset(pmap);
  pool_free(P_PMAP, pmap);
}

/*! \brief Inserts the TLB entry mapping \a vaddr into the TLB. */
static inline void pmap_ensure_safe_pt_access(pmap_t *pmap, vaddr_t vaddr) {
  tlbhi_t hi = mips32_getentryhi();
  uintptr_t pte_addr = (uintptr_t)&PTE_OF(pmap, vaddr);
  tlbentry_t temp = {.hi = PTE_ASID(hi) | PTE_VPN2(pte_addr),
                     .lo0 = PDE_OF(pmap, vaddr & ~(1 << PDE_SHIFT)),
                     .lo1 = PDE_OF(pmap, vaddr | (1 << PDE_SHIFT))};
  tlb_overwrite_random(&temp);
}

/*
 * pmap_pte_write calls pmap_add_pde, and pmap_add_pde calls
 * pmap_pte_write, so we need this declaration.
 */
static void pmap_add_pde(pmap_t *pmap, vaddr_t vaddr);

/*! \brief Reads the PTE mapping virtual address \a vaddr.
 *
 * Reads the PTE mapping virtual address \a vaddr from \a pmap.
 * The Page Table access is guaranteed not to generate a TLB miss.
 */
static pte_t pmap_pte_read(pmap_t *pmap, vaddr_t vaddr) {
  if (!is_valid(PDE_OF(pmap, vaddr)))
    return 0;
  /* Interrupt handlers could generate TLB misses. */
  SCOPED_INTR_DISABLED();
  /*
   * ptep can't be read from the stack after returning from
   * pmap_ensure_safe_pt_access, as that could generate a TLB miss.
   */
  register pte_t *ptep = &PTE_OF(pmap, vaddr);
  pmap_ensure_safe_pt_access(pmap, vaddr);
  return *ptep;
}

/*! \brief Writes \a pte as the new PTE mapping virtual address \a vaddr.
 *
 * Writes \a pte as the new PTE mapping virtual address \a vaddr in
 * \a pmap. The Page Table access is guaranteed not to generate a TLB miss.
 */
static void pmap_pte_write(pmap_t *pmap, vaddr_t vaddr, pte_t pte) {
  if (!is_valid(PDE_OF(pmap, vaddr)))
    pmap_add_pde(pmap, vaddr);
  SCOPED_INTR_DISABLED();
  register pte_t *ptep = &PTE_OF(pmap, vaddr);
  pmap_ensure_safe_pt_access(pmap, vaddr);
  *ptep = pte;
  tlb_invalidate(PTE_VPN2(vaddr) | PTE_ASID(pmap->asid));
}

/* Internal use, assumes pmap is locked. */
static bool pmap_is_range_mapped(pmap_t *pmap, vaddr_t start, vaddr_t end) {
  vaddr_t addr;

  for (addr = start; addr < end; addr += PD_ENTRIES * PAGESIZE)
    if (!is_valid(PDE_OF(pmap, addr)))
      return false;

  for (addr = start; addr < end; addr += PTF_ENTRIES * PAGESIZE)
    if (!is_valid(pmap_pte_read(pmap, addr)))
      return false;

  return true;
}

/* Add PT to PD so kernel can handle access to @vaddr. */
static void pmap_add_pde(pmap_t *pmap, vaddr_t vaddr) {
  /* assume page table fragment not present in physical memory */
  assert(!is_valid(PDE_OF(pmap, vaddr)));

  vm_page_t *pg = pm_alloc(1);
  TAILQ_INSERT_TAIL(&pmap->pte_pages, pg, pageq);
  klog("Page table fragment %08lx allocated at %08lx", PTF_ADDR_OF(vaddr),
       pg->paddr);

  PDE_OF(pmap, vaddr) = PTE_PFN(pg->paddr) | PTE_KERNEL;

  vaddr_t addr = vaddr & ~((1 << PDE_SHIFT) - 1);
  vaddr_t end = addr + (1 << PDE_SHIFT);

  /*
   * We don't want to call pmap_pte_write with a PD address,
   * as a call to tlb_invalidate would overwrite wired TLB entries!
   */
  if (addr == PD_BASE)
    addr = PD_BASE + 2 * PD_SIZE;

  while (addr < end) {
    pmap_pte_write(pmap, addr, PTE_GLOBAL);
    addr += PAGESIZE;
  }
}

/* TODO: implement */
void pmap_remove_pde(pmap_t *pmap, vaddr_t vaddr);

#if 0
/* Used if CPU implements RI and XI bits in ENTRYLO. */
static pte_t vm_prot_map[] = {
  [VM_PROT_NONE] = 0,
  [VM_PROT_READ] = PTE_VALID|PTE_NO_EXEC,
  [VM_PROT_WRITE] = PTE_VALID|PTE_DIRTY|PTE_NO_READ|PTE_NO_EXEC,
  [VM_PROT_READ|VM_PROT_WRITE] = PTE_VALID|PTE_DIRTY|PTE_NO_EXEC,
  [VM_PROT_EXEC] = PTE_VALID|PTE_NO_READ,
  [VM_PROT_READ|VM_PROT_EXEC] = PTE_VALID,
  [VM_PROT_WRITE|VM_PROT_EXEC] = PTE_VALID|PTE_DIRTY|PTE_NO_READ,
  [VM_PROT_READ|VM_PROT_WRITE|VM_PROT_EXEC] = PTE_VALID|PTE_DIRTY,
};
#else
static pte_t vm_prot_map[] = {
    [VM_PROT_NONE] = 0,
    [VM_PROT_READ] = PTE_VALID,
    [VM_PROT_WRITE] = PTE_VALID | PTE_DIRTY,
    [VM_PROT_READ | VM_PROT_WRITE] = PTE_VALID | PTE_DIRTY,
    [VM_PROT_EXEC] = PTE_VALID,
    [VM_PROT_READ | VM_PROT_EXEC] = PTE_VALID,
    [VM_PROT_WRITE | VM_PROT_EXEC] = PTE_VALID | PTE_DIRTY,
    [VM_PROT_READ | VM_PROT_WRITE | VM_PROT_EXEC] = PTE_VALID | PTE_DIRTY,
};
#endif

/* TODO: what about caches? */
static void pmap_set_pte(pmap_t *pmap, vaddr_t vaddr, paddr_t paddr,
                         vm_prot_t prot) {
  pmap_pte_write(pmap, vaddr, PTE_PFN(paddr) | vm_prot_map[prot] |
                                (in_kernel_space(vaddr) ? PTE_GLOBAL : 0));
  klog("Add mapping for page %08lx (PTE at %08lx)", (vaddr & PTE_MASK),
       (vaddr_t)&PTE_OF(pmap, vaddr));
}

/* TODO: what about caches? */
static void pmap_clear_pte(pmap_t *pmap, vaddr_t vaddr) {
  pmap_pte_write(pmap, vaddr, 0);
  klog("Remove mapping for page %08lx (PTE at %08lx)", (vaddr & PTE_MASK),
       (vaddr_t)&PTE_OF(pmap, vaddr));

  /* TODO: Deallocate empty page table fragment by calling pmap_remove_pde. */
}

/* TODO: what about caches? */
static void pmap_change_pte(pmap_t *pmap, vaddr_t vaddr, vm_prot_t prot) {
  pmap_pte_write(pmap, vaddr, (pmap_pte_read(pmap, vaddr) & ~PTE_PROT_MASK) |
                                vm_prot_map[prot]);
  klog("Change protection bits for page %08lx (PTE at %08lx)",
       (vaddr & PTE_MASK), (vaddr_t)&PTE_OF(pmap, vaddr));
}

void pmap_enter(pmap_t *pmap, vaddr_t start, vaddr_t end, paddr_t paddr,
                vm_prot_t prot) {
  assert(is_aligned(start, PAGESIZE) && is_aligned(end, PAGESIZE));
  assert(start < end && start >= pmap->start && end <= pmap->end);
  assert(is_aligned(paddr, PAGESIZE));
  SCOPED_MTX_LOCK(&pmap->mtx);
  assert(!pmap_is_range_mapped(pmap, start, end));

  while (start < end) {
    pmap_set_pte(pmap, start, paddr, prot);
    start += PAGESIZE, paddr += PAGESIZE;
  }
}

void pmap_remove(pmap_t *pmap, vaddr_t start, vaddr_t end) {
  assert(is_aligned(start, PAGESIZE) && is_aligned(end, PAGESIZE));
  assert(start < end && start >= pmap->start && end <= pmap->end);
  SCOPED_MTX_LOCK(&pmap->mtx);
  assert(pmap_is_range_mapped(pmap, start, end));

  while (start < end) {
    pmap_clear_pte(pmap, start);
    start += PAGESIZE;
  }
}

void pmap_protect(pmap_t *pmap, vaddr_t start, vaddr_t end, vm_prot_t prot) {
  assert(is_aligned(start, PAGESIZE) && is_aligned(end, PAGESIZE));
  assert(start < end && start >= pmap->start && end <= pmap->end);
  SCOPED_MTX_LOCK(&pmap->mtx);
  assert(pmap_is_range_mapped(pmap, start, end));

  while (start < end) {
    pmap_change_pte(pmap, start, prot);
    start += PAGESIZE;
  }
}

void pmap_zero_page(vm_page_t *pg) {
  bzero(PG_KSEG0_ADDR(pg), PAGESIZE);
}

void pmap_copy_page(vm_page_t *src, vm_page_t *dst) {
  memcpy(PG_KSEG0_ADDR(dst), PG_KSEG0_ADDR(src), PAGESIZE);
}

/* TODO: at any given moment there're two page tables in use:
 *  - kernel-space pmap for kseg2 & kseg3
 *  - user-space pmap for useg
 * ks_pmap is shared across all vm_maps. How to switch us_pmap efficiently?
 * Correct solution needs to make sure no unwanted entries are left in TLB and
 * caches after the switch.
 */

void pmap_activate(pmap_t *pmap) {
  SCOPED_NO_PREEMPTION();

  PCPU_GET(curpmap) = pmap;
  update_wired_pde(pmap);

  /* Set ASID for current process */
  mips32_setentryhi(pmap ? pmap->asid : 0);
}

pmap_t *get_kernel_pmap() {
  return &kernel_pmap;
}

pmap_t *get_user_pmap() {
  return PCPU_GET(curpmap);
}

pmap_t *get_active_pmap_by_addr(vaddr_t addr) {
  if (in_kernel_space(addr))
    return get_kernel_pmap();
  if (in_user_space(addr))
    return get_user_pmap();
  return NULL;
}

void tlb_exception_handler(exc_frame_t *frame) {
  thread_t *td = thread_self();

  int code = (frame->cause & CR_X_MASK) >> CR_X_SHIFT;
  vaddr_t vaddr = frame->badvaddr;

  klog("%s at $%08x, caused by reference to $%08lx!", exceptions[code],
       frame->pc, vaddr);

  /* Accesses to the page table should never go beyond tlb_refill. */
  assert(vaddr < PT_BASE || PT_BASE + PT_SIZE + 2 * PAGESIZE <= vaddr);

  vm_map_t *map = get_active_vm_map_by_addr(vaddr);
  if (!map) {
    klog("No virtual address space defined for %08lx!", vaddr);
    goto fault;
  }
  vm_prot_t access = (code == EXC_TLBL) ? VM_PROT_READ : VM_PROT_WRITE;
  intr_enable();
  int ret = vm_page_fault(map, vaddr, access);
  intr_disable();
  if (ret == 0)
    return;

fault:
  if (td->td_onfault) {
    /* handle copyin / copyout faults */
    frame->pc = td->td_onfault;
    td->td_onfault = 0;
  } else if (td->td_proc) {
    /* Panic when process running in kernel space uses wrong pointer. */
    if (in_kernel_mode(frame))
      kernel_oops(frame);

    /* Send a segmentation fault signal to the user program. */

    /* TODO it's an awful kludge,
     * once pmap & vm_map is properly synchronized it will be removed
     * and whole tlb_exception_handler will run as preemptible code */
    intr_enable();
    sig_send(td->td_proc, SIGSEGV);
    intr_disable();
  } else if (ktest_test_running_flag) {
    ktest_failure();
  } else {
    /* Panic when kernel-mode thread uses wrong pointer. */
    kernel_oops(frame);
  }
}
