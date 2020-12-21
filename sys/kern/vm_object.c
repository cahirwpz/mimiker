#define KL_LOG KL_VM
#include <sys/klog.h>
#include <sys/mimiker.h>
#include <sys/pool.h>
#include <sys/pmap.h>
#include <sys/vm_object.h>
#include <sys/vm_physmem.h>

static POOL_DEFINE(P_VMOBJ, "vm_object", sizeof(vm_object_t));

vm_object_t *vm_object_alloc(vm_pgr_type_t type) {
  vm_object_t *obj = pool_alloc(P_VMOBJ, M_ZERO);
  TAILQ_INIT(&obj->list);
  TAILQ_INIT(&obj->shadows_list);
  rw_init(&obj->mtx, NULL, 0);
  obj->pager = &pagers[type];
  obj->ref_counter = 1;
  return obj;
}

vm_page_t *vm_object_find_page_nolock(vm_object_t *obj, off_t offset) {
  vm_page_t *pg;
  TAILQ_FOREACH (pg, &obj->list, obj.list) {
    if (pg->offset == offset)
      return pg;
  }

  return NULL;
}

vm_page_t *vm_object_find_page(vm_object_t *obj, off_t offset) {
  SCOPED_RW_ENTER(&obj->mtx, RW_READER);
  return vm_object_find_page_nolock(obj, offset);
}

void vm_object_add_page_nolock(vm_object_t *obj, off_t offset, vm_page_t *pg) {
  assert(page_aligned_p(pg->offset));
  /* For simplicity of implementation let's insert pages of size 1 only */
  assert(pg->size == 1);

  pg->object = obj;
  pg->offset = offset;

  refcnt_acquire(&pg->ref_counter);

  vm_page_t *it;
  TAILQ_FOREACH (it, &obj->list, obj.list) {
    if (it->offset > pg->offset) {
      TAILQ_INSERT_BEFORE(it, pg, obj.list);
      obj->npages++;
      return;
    }
    /* there must be no page at the offset! */
    assert(it->offset != pg->offset);
  }

  /* offset of page is greater than the offset of any other page */
  TAILQ_INSERT_TAIL(&obj->list, pg, obj.list);
  obj->npages++;
}

void vm_object_add_page(vm_object_t *obj, off_t offset, vm_page_t *pg) {
  SCOPED_RW_ENTER(&obj->mtx, RW_WRITER);
  vm_object_add_page_nolock(obj, offset, pg);
}

static void vm_object_remove_page_nolock(vm_object_t *obj, vm_page_t *page) {
  page->offset = 0;
  page->object = NULL;

  TAILQ_REMOVE(&obj->list, page, obj.list);
  if (refcnt_release(&page->ref_counter)) {
    vm_page_free(page);
  }

  obj->npages--;
}

void vm_object_remove_page(vm_object_t *obj, vm_page_t *page) {
  SCOPED_RW_ENTER(&obj->mtx, RW_WRITER);
  vm_object_remove_page_nolock(obj, page);
}

void vm_object_remove_range(vm_object_t *object, off_t offset, size_t length) {
  SCOPED_RW_ENTER(&object->mtx, RW_WRITER);

  vm_page_t *pg, *next;
  TAILQ_FOREACH_SAFE (pg, &object->list, obj.list, next) {
    if (pg->offset >= (off_t)(offset + length))
      break;
    if (pg->offset >= offset)
      vm_object_remove_page_nolock(object, pg);
  }
}

static void merge_shadow(vm_object_t *shadow) {
  vm_object_t *elem;

  SCOPED_RW_ENTER(&shadow->mtx, RW_WRITER);

  TAILQ_FOREACH (elem, &shadow->shadows_list, link) {
    assert(elem != NULL);

    SCOPED_RW_ENTER(&elem->mtx, RW_WRITER);

    vm_page_t *pg;
    TAILQ_FOREACH (pg, &shadow->list, obj.list) {
      if (vm_object_find_page_nolock(elem, pg->offset) == NULL) {
        /* here releasing the counter for pg is missing */
        TAILQ_REMOVE(&shadow->list, pg, obj.list);
        vm_object_add_page_nolock(elem, pg->offset, pg);
      }
    }

    elem->backing_object = shadow->backing_object;
    elem->pager = shadow->pager;

    if (elem->backing_object) {
      WITH_RW_LOCK (&elem->backing_object->mtx, RW_WRITER) {
        refcnt_acquire(&elem->backing_object->ref_counter);
        /* here can be the problem with reference counters in pages */
        /* also maybe in case when we free an object that has the shadow object
         */
        /* because we decrease the reference counter for shadow, but not for his
         * pages */
        TAILQ_INSERT_HEAD(&elem->backing_object->shadows_list, elem, link);
      }
    }
  }
}

void vm_object_free(vm_object_t *obj) {
  WITH_RW_LOCK (&obj->mtx, RW_WRITER) {
    if (!refcnt_release(&obj->ref_counter)) {
      return;
    }

    vm_page_t *pg, *next;
    TAILQ_FOREACH_SAFE (pg, &obj->list, obj.list, next)
      vm_object_remove_page_nolock(obj, pg);

    if (obj->backing_object) {
      vm_object_t *shadow = obj->backing_object;
      TAILQ_REMOVE(&shadow->shadows_list, obj, link);

      if (shadow->ref_counter == 2) {
        merge_shadow(shadow);
        vm_object_free(obj->backing_object);
      }

      vm_object_free(obj->backing_object);
    }
  }

  pool_free(P_VMOBJ, obj);
}

vm_object_t *vm_object_clone(vm_object_t *obj) {
  vm_object_t *new_obj = vm_object_alloc(VM_DUMMY);
  new_obj->pager = obj->pager;
  SCOPED_RW_ENTER(&obj->mtx, RW_READER);

  vm_page_t *pg;
  TAILQ_FOREACH (pg, &obj->list, obj.list) {
    vm_page_t *new_pg = vm_page_alloc(1);
    pmap_copy_page(pg, new_pg);
    vm_object_add_page(new_obj, pg->offset, new_pg);
  }

  return new_obj;
}

void vm_map_object_dump(vm_object_t *obj) {
  SCOPED_RW_ENTER(&obj->mtx, RW_READER);
  vm_page_t *pg;
  TAILQ_FOREACH (pg, &obj->list, obj.list) {
    klog("(vm-obj) offset: 0x%08lx, size: %ld", pg->offset, pg->size);
  }
}

void vm_object_set_prot(vm_object_t *obj, vm_prot_t prot) {
  SCOPED_RW_ENTER(&obj->mtx, RW_WRITER);

  vm_page_t *pg;
  TAILQ_FOREACH (pg, &obj->list, obj.list) { pmap_set_page_prot(pg, prot); }
}

void vm_object_increase_pages_references(vm_object_t *obj) {
  SCOPED_RW_ENTER(&obj->mtx, RW_WRITER);

  vm_page_t *pg;
  TAILQ_FOREACH (pg, &obj->list, obj.list) { refcnt_acquire(&pg->ref_counter); }

  if (obj->backing_object) {
    vm_object_increase_pages_references(obj->backing_object);
  }
}