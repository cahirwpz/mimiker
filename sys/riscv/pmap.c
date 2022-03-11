#define KL_LOG KL_PMAP
#include <sys/kenv.h>
#include <sys/klog.h>
#include <sys/mimiker.h>
#include <sys/pmap.h>
#include <riscv/pmap.h>
#include <riscv/pte.h>

/* Physical memory boundaries. */
static paddr_t dmap_paddr_base;
static paddr_t dmap_paddr_end;

static paddr_t kernel_pde;

bool pmap_address_p(pmap_t *pmap, vaddr_t va) {
  panic("Not implemented!");
}

bool pmap_contains_p(pmap_t *pmap, vaddr_t start, vaddr_t end) {
  panic("Not implemented!");
}

vaddr_t pmap_start(pmap_t *pmap) {
  panic("Not implemented!");
}

vaddr_t pmap_end(pmap_t *pmap) {
  panic("Not implemented!");
}

void init_pmap(void) {
  panic("Not implemented!");
}

pmap_t *pmap_new(void) {
  panic("Not implemented!");
}

void pmap_delete(pmap_t *pmap) {
  panic("Not implemented!");
}

void pmap_bootstrap(paddr_t pd_pa, vaddr_t pd_va) {
  uint32_t dmap_size = kenv_get_ulong("mem_size");

  /* Obtain basic parameters. */
  dmap_paddr_base = kenv_get_ulong("mem_start");
  dmap_paddr_end = dmap_paddr_base + dmap_size;
  kernel_pde = pd_pa;

  /* Assume physical memory starts at the beginning of L0 region. */
  assert(is_aligned(dmap_paddr_base, L0_SIZE));

  /* We must have enough virtual addresses. */
  assert(dmap_size <= DMAP_MAX_SIZE);

  /* We assume 32-bit physical address space. */
  assert(dmap_paddr_base < dmap_paddr_end);

  klog("Physical memory range: %p - %p", dmap_paddr_base, dmap_paddr_end - 1);

  klog("dmap range: %p - %p", DMAP_VADDR_BASE, DMAP_VADDR_BASE + dmap_size - 1);

  /* Build direct map using 4MiB superpages. */
  pd_entry_t *pde = (void *)pd_va;
  size_t idx = L0_INDEX(DMAP_VADDR_BASE);
  for (paddr_t pa = dmap_paddr_base; pa < dmap_paddr_end; pa += L0_SIZE, idx++)
    pde[idx] = PA_TO_PTE(pa) | PTE_KERN;
}

void pmap_enter(pmap_t *pmap, vaddr_t va, vm_page_t *pg, vm_prot_t prot,
                unsigned flags) {
  panic("Not implemented!");
}

bool pmap_extract(pmap_t *pmap, vaddr_t va, paddr_t *pap) {
  panic("Not implemented!");
}

void pmap_remove(pmap_t *pmap, vaddr_t start, vaddr_t end) {
  panic("Not implemented!");
}

void pmap_kenter(vaddr_t va, paddr_t pa, vm_prot_t prot, unsigned flags) {
  panic("Not implemented!");
}

bool pmap_kextract(vaddr_t va, paddr_t *pap) {
  panic("Not implemented!");
}

void pmap_kremove(vaddr_t va, size_t size) {
  panic("Not implemented!");
}

void pmap_protect(pmap_t *pmap, vaddr_t start, vaddr_t end, vm_prot_t prot) {
  panic("Not implemented!");
}

void pmap_page_remove(vm_page_t *pg) {
  panic("Not implemented!");
}

void pmap_zero_page(vm_page_t *pg) {
  panic("Not implemented!");
}

void pmap_copy_page(vm_page_t *src, vm_page_t *dst) {
  panic("Not implemented!");
}

bool pmap_clear_modified(vm_page_t *pg) {
  panic("Not implemented!");
}

bool pmap_clear_referenced(vm_page_t *pg) {
  panic("Not implemented!");
}

bool pmap_is_modified(vm_page_t *pg) {
  panic("Not implemented!");
}

bool pmap_is_referenced(vm_page_t *pg) {
  panic("Not implemented!");
}

void pmap_set_referenced(vm_page_t *pg) {
  panic("Not implemented!");
}

void pmap_set_modified(vm_page_t *pg) {
  panic("Not implemented!");
}

int pmap_emulate_bits(pmap_t *pmap, vaddr_t va, vm_prot_t prot) {
  panic("Not implemented!");
}

void pmap_activate(pmap_t *pmap) {
  panic("Not implemented!");
}

pmap_t *pmap_lookup(vaddr_t va) {
  panic("Not implemented!");
}

pmap_t *pmap_kernel(void) {
  panic("Not implemented!");
}

pmap_t *pmap_user(void) {
  panic("Not implemented!");
}

void pmap_growkernel(vaddr_t maxkvaddr) {
  panic("Not implemented!");
}
