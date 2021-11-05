#define KL_KOL KL_PMAP
#include <sys/klog.h>
#include <sys/mimiker.h>
#include <sys/pmap.h>
#include <riscv/pmap.h>

/* Kernel page directory. */
static pd_entry_t *kernel_pde;

/* Physical memory start. */
static paddr_t dmap_paddr_base;

/* Physical memory size. */
static size_t dmap_size;

bool pmap_address_p(pmap_t *pmap, vaddr_t va) {
  panic("Not implemented!\n");
}

bool pmap_contains_p(pmap_t *pmap, vaddr_t start, vaddr_t end) {
  panic("Not implemented!\n");
}

vaddr_t pmap_start(pmap_t *pmap) {
  panic("Not implemented!\n");
}

vaddr_t pmap_end(pmap_t *pmap) {
  panic("Not implemented!\n");
}

void init_pmap(void) {
  panic("Not implemented!\n");
}

pmap_t *pmap_new(void) {
  panic("Not implemented!\n");
}

void pmap_delete(pmap_t *pmap) {
  panic("Not implemented!\n");
}

int pmap_bootstrap(pd_entry_t *pde, paddr_t min_pa, size_t pm_size) {
  /* Save bootstrap parameters. */
  dmap_paddr_base = min_pa;
  dmap_size = pm_size;
  kernel_pde = pde;

  if (!is_aligned(min_pa, L1_SIZE))
    return 1;

  /* Do we have enough virtual addresses? */
  if (pm_size > DMAP_MAX_SIZE)
    return 1;

  /* Build a direct map using 4MiB superpages. */
  size_t idx = L0_INDEX(DMAP_VADDR_BASE);
  uint64_t max_pa = (uint64_t)min_pa + pm_size;
  for (paddr_t pa = min_pa; pa < max_pa; pa += L1_SIZE, idx++)
    pde[idx] = PA_TO_PTE(pa) | PTE_R | PTE_W | PTE_KERN;

  return 0;
}

void pmap_enter(pmap_t *pmap, vaddr_t va, vm_page_t *pg, vm_prot_t prot,
                unsigned flags) {
  panic("Not implemented!\n");
}

bool pmap_extract(pmap_t *pmap, vaddr_t va, paddr_t *pap) {
  panic("Not implemented!\n");
}

void pmap_remove(pmap_t *pmap, vaddr_t start, vaddr_t end) {
  panic("Not implemented!\n");
}

void pmap_kenter(vaddr_t va, paddr_t pa, vm_prot_t prot, unsigned flags) {
  panic("Not implemented!\n");
}

bool pmap_kextract(vaddr_t va, paddr_t *pap) {
  panic("Not implemented!\n");
}

void pmap_kremove(vaddr_t va, size_t size) {
  panic("Not implemented!\n");
}

void pmap_protect(pmap_t *pmap, vaddr_t start, vaddr_t end, vm_prot_t prot) {
  panic("Not implemented!\n");
}

void pmap_page_remove(vm_page_t *pg) {
  panic("Not implemented!\n");
}

void pmap_zero_page(vm_page_t *pg) {
  panic("Not implemented!\n");
}

void pmap_copy_page(vm_page_t *src, vm_page_t *dst) {
  panic("Not implemented!\n");
}

bool pmap_clear_modified(vm_page_t *pg) {
  panic("Not implemented!\n");
}

bool pmap_clear_referenced(vm_page_t *pg) {
  panic("Not implemented!\n");
}

bool pmap_is_modified(vm_page_t *pg) {
  panic("Not implemented!\n");
}

bool pmap_is_referenced(vm_page_t *pg) {
  panic("Not implemented!\n");
}

void pmap_set_referenced(vm_page_t *pg) {
  panic("Not implemented!\n");
}

void pmap_set_modified(vm_page_t *pg) {
  panic("Not implemented!\n");
}

int pmap_emulate_bits(pmap_t *pmap, vaddr_t va, vm_prot_t prot) {
  panic("Not implemented!\n");
}

void pmap_activate(pmap_t *pmap) {
  panic("Not implemented!\n");
}

pmap_t *pmap_lookup(vaddr_t va) {
  panic("Not implemented!\n");
}

pmap_t *pmap_kernel(void) {
  panic("Not implemented!\n");
}

pmap_t *pmap_user(void) {
  panic("Not implemented!\n");
}

void pmap_growkernel(vaddr_t maxkvaddr) {
  panic("Not implemented!\n");
}
