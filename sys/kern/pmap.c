#define KL_LOG KL_PMAP
#include <sys/klog.h>
#include <sys/pmap.h>
#include <sys/vm.h>
#include <sys/vm_physmem.h>

bool pmap_clear_referenced(vm_page_t *pg) {
  bool prev = pmap_is_referenced(pg);
  pg->flags &= ~PG_REFERENCED;
  return prev;
}

bool pmap_clear_modified(vm_page_t *pg) {
  bool prev = pmap_is_modified(pg);
  pg->flags &= ~PG_MODIFIED;
  return prev;
}

bool pmap_is_referenced(vm_page_t *pg) {
  return pg->flags & PG_REFERENCED;
}

bool pmap_is_modified(vm_page_t *pg) {
  return pg->flags & PG_MODIFIED;
}

void pmap_set_referenced(paddr_t pa) {
  vm_page_t *pg = vm_page_find(pa);
  pg->flags |= PG_REFERENCED;
}

void pmap_set_modified(paddr_t pa) {
  vm_page_t *pg = vm_page_find(pa);
  pg->flags |= PG_MODIFIED;
}
