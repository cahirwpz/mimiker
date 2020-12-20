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
  assert(obj->backing_object != NULL);

  vm_page_t *pg = NULL;
  vm_page_t *new_pg = NULL;

  vm_object_t *it = obj;
  vm_object_t *prev = obj;

  WITH_RW_LOCK (&obj->mtx, RW_READER) {

    while (pg == NULL && it->backing_object != NULL) {
      SCOPED_RW_ENTER(&it->backing_object->mtx, RW_READER);
      pg = vm_object_find_page(it->backing_object, offset);
      prev = it;
      it = it->backing_object;
    }
  }

  if (pg == NULL) {
    new_pg = it->pager->pgr_fault(obj, offset);
  } else {
    if (!refcnt_release(&pg->ref_counter)) {
      new_pg = vm_page_alloc(1);
      pmap_copy_page(pg, new_pg);
    } else {
      new_pg = pg;

      refcnt_acquire(&pg->ref_counter);
      refcnt_acquire(&pg->ref_counter);

      vm_object_remove_page(it, pg);

      refcnt_release(&pg->ref_counter);

      if (it->npages == 0) {
        prev->backing_object = it->backing_object;
        prev->pager = it->pager;
        if (it->backing_object != NULL) {
          refcnt_acquire(&it->backing_object->ref_counter);
        }

        vm_object_free(it);
      }
    }
  }

  vm_object_add_page(obj, offset, new_pg);

  return new_pg;
}

vm_pager_t pagers[] = {
  [VM_DUMMY] = {.pgr_fault = dummy_pager_fault},
  [VM_ANONYMOUS] = {.pgr_fault = anon_pager_fault},
  [VM_SHADOW] = {.pgr_fault = shadow_pager_fault},
};
