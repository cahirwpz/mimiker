#define KL_KOL KL_PMAP
#include <sys/kenv.h>
#include <sys/klog.h>
#include <sys/mimiker.h>
#include <sys/pmap.h>
#include <riscv/pmap.h>

/* Kernel page directory. */
static paddr_t kernel_pde;

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

void pmap_bootstrap(paddr_t pd_pa, pd_entry_t *pd_va) {
  /* Obtain basic parameters. */
  dmap_paddr_base = kenv_get_ulong("mem_start");
  dmap_size = kenv_get_ulong("mem_size");
  kernel_pde = pd_pa;

  /* Assume the physical memory starts at the beginning of an L0 region. */
  assert(is_aligned(dmap_paddr_base, L0_SIZE));

  /* We must have enough virtual addresses. */
  assert(dmap_size <= DMAP_MAX_SIZE);

  uint32_t min_pa = dmap_paddr_base;
  uint32_t max_pa = min_pa + dmap_size;

  /* We assume maximum physical address < 2^((sizeof(paddr_t) * 8)). */
  assert(min_pa < max_pa);

  /* Build a direct map using 4MiB superpages. */
  size_t idx = L0_INDEX(DMAP_VADDR_BASE);
  for (paddr_t pa = min_pa; pa < max_pa; pa += L0_SIZE, idx++)
    pd_va[idx] = PA_TO_PTE(pa) | PTE_KERN;
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
