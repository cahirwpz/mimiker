#include <sys/mimiker.h>
#include <sys/pmap.h>
#include <sys/vm_object.h>
#include <sys/vm_pager.h>
#include <sys/vm_physmem.h>

static vm_page_t *dummy_pager_fault(vm_object_t *obj, off_t offset) {
  return NULL;
}

static vm_page_t *anon_pager_fault(vm_object_t *obj, off_t offset) {
  assert(obj != NULL);

  vm_page_t *new_pg = vm_page_alloc(1);
  pmap_zero_page(new_pg);
  vm_object_add_page(obj, offset, new_pg);
  return new_pg;
}

static vm_page_t *shadow_pager_fault(vm_object_t *obj, off_t offset) {
  assert(obj != NULL);
  assert(obj->shadow_object != NULL);

  vm_page_t *new_pg = vm_object_find_page(obj->shadow_object, offset);

  if (new_pg == NULL) {
    new_pg = obj->shadow_object->pager->pgr_fault(obj, offset);
  }

  assert(new_pg != NULL);

  vm_object_add_page(obj, offset, new_pg);

  return new_pg;
}

vm_pager_t pagers[] = {
  [VM_DUMMY] = {.pgr_fault = dummy_pager_fault},
  [VM_ANONYMOUS] = {.pgr_fault = anon_pager_fault},
  [VM_SHADOW] = {.pgr_fault = shadow_pager_fault},
};
