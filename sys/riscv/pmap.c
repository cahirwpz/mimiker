#define KL_KOL KL_PMAP
#include <sys/klog.h>
#include <sys/pmap.h>

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
