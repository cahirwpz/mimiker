#define KL_LOG KL_PMAP
#include <sys/klog.h>
#include <sys/mimiker.h>
#include <aarch64/exception.h>
#include <aarch64/pmap.h>
#include <sys/pmap.h>
#include <sys/vm.h>

/* Kernel page directory entries. */
alignas(PAGESIZE) pde_t _kernel_pmap_pde[PD_ENTRIES];

static pmap_t kernel_pmap;

/* TODO(pj) */
static asid_t alloc_asid(void) {
  int free = 0;
  klog("alloc_asid() = %d", free);
  return free;
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
  panic("Not implemented!");
}

void pmap_delete(pmap_t *pmap) {
  panic("Not implemented!");
}

void pmap_kenter(paddr_t va, paddr_t pa, vm_prot_t prot, unsigned flags) {
  panic("Not implemented!");
}

void pmap_enter(pmap_t *pmap, vaddr_t va, vm_page_t *pg, vm_prot_t prot,
                unsigned flags) {
  panic("Not implemented!");
}

void pmap_kremove(vaddr_t start, vaddr_t end) {
  panic("Not implemented!");
}

void pmap_remove(pmap_t *pmap, vaddr_t start, vaddr_t end) {
  panic("Not implemented!");
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
  panic("Not implemented!");
}

pmap_t *pmap_kernel(void) {
  panic("Not implemented!");
}

pmap_t *pmap_user(void) {
  panic("Not implemented!");
}

pmap_t *pmap_lookup(vaddr_t addr) {
  panic("Not implemented!");
}

void tlb_exception_handler(exc_frame_t *frame) {
  panic("Not implemented!");
}
