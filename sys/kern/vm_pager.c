#include <sys/mimiker.h>
#include <sys/pmap.h>
#include <sys/vm_object.h>
#include <sys/vm_pager.h>
#include <sys/vm_physmem.h>

static vm_page_t *dummy_pager_fault(vm_object_t *obj, vm_offset_t offset) {
  return NULL;
}

static vm_page_t *anon_pager_fault(vm_object_t *obj, vm_offset_t offset) {
  assert(obj != NULL);

  vm_page_t *new_pg = vm_page_alloc(1);
  pmap_zero_page(new_pg);
  vm_object_add_page(obj, offset, new_pg);
  return new_pg;
}

vm_pager_t pagers[] = {
  [VM_PGR_DUMMY] = {.pgr_type = VM_PGR_DUMMY, .pgr_fault = dummy_pager_fault},
  [VM_PGR_ANONYMOUS] = {.pgr_type = VM_PGR_ANONYMOUS,
                        .pgr_fault = anon_pager_fault},
};
