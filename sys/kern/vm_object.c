#define KL_LOG KL_VM
#include <sys/klog.h>
#include <sys/mimiker.h>
#include <sys/pool.h>
#include <sys/pmap.h>
#include <sys/vm_object.h>
#include <sys/vm_physmem.h>

static POOL_DEFINE(P_VMOBJ, "vm_object", sizeof(vm_object_t));

static inline int vm_page_cmp(vm_page_t *a, vm_page_t *b) {
  if (a->offset < b->offset)
    return -1;
  return a->offset - b->offset;
}

vm_object_t *vm_object_alloc(vm_pgr_type_t type) {
  vm_object_t *obj = pool_alloc(P_VMOBJ, M_ZERO);
  TAILQ_INIT(&obj->list);
  mtx_init(&obj->mtx, 0);
  obj->pager = &pagers[type];
  obj->ref_counter = 1;
  return obj;
}

vm_page_t *vm_object_find_page(vm_object_t *obj, off_t offset) {
  vm_page_t find = {.offset = offset};
  SCOPED_MTX_LOCK(&obj->mtx);

  vm_page_t *pg;

  TAILQ_FOREACH (pg, &obj->list, obj.list) {
    if (vm_page_cmp(&find, pg) == 0) {
      return pg;
    }
  }

  return NULL;
}

bool vm_object_add_page(vm_object_t *obj, off_t offset, vm_page_t *page) {
  assert(page_aligned_p(page->offset));
  /* For simplicity of implementation let's insert pages of size 1 only */
  assert(page->size == 1);

  page->object = obj;
  page->offset = offset;

  SCOPED_MTX_LOCK(&obj->mtx);
  vm_page_t *pg;

  TAILQ_FOREACH (pg, &obj->list, obj.list) {
    if (vm_page_cmp(page, pg) < 0) {
      TAILQ_INSERT_BEFORE(pg, page, obj.list);
      obj->npages++;
      return true;
    } else if (vm_page_cmp(pg, page) == 0) {
      /* page at offset we are trying to add already exists in the object */
      return false;
    }
  }

  /* offset of page is greater than the offset of any other page */
  TAILQ_INSERT_TAIL(&obj->list, page, obj.list);
  obj->npages++;

  return true;
}

static void vm_object_remove_page_nolock(vm_object_t *obj, vm_page_t *page) {
  page->offset = 0;
  page->object = NULL;

  TAILQ_REMOVE(&obj->list, page, obj.list);
  vm_page_free(page);
  obj->npages--;
}

void vm_object_remove_page(vm_object_t *obj, vm_page_t *page) {
  SCOPED_MTX_LOCK(&obj->mtx);

  vm_object_remove_page_nolock(obj, page);
}

void vm_object_remove_range(vm_object_t *object, off_t offset, size_t length) {
  vm_page_t *pg, *next;

  SCOPED_MTX_LOCK(&object->mtx);

  TAILQ_FOREACH_SAFE (pg, &object->list, obj.list, next) {
    if (pg->offset >= (off_t)(offset + length))
      break;
    if (pg->offset >= offset)
      vm_object_remove_page_nolock(object, pg);
  }
}

void vm_object_free(vm_object_t *obj) {
  WITH_MTX_LOCK (&obj->mtx) {
    if (!refcnt_release(&obj->ref_counter)) {
      return;
    }

    while (!TAILQ_EMPTY(&obj->list)) {
      vm_page_t *pg = TAILQ_FIRST(&obj->list);
      vm_object_remove_page_nolock(obj, pg);
    }
  }
  pool_free(P_VMOBJ, obj);
}

vm_object_t *vm_object_clone(vm_object_t *obj) {
  vm_object_t *new_obj = vm_object_alloc(VM_DUMMY);
  new_obj->pager = obj->pager;

  SCOPED_MTX_LOCK(&obj->mtx);

  vm_page_t *pg;
  TAILQ_FOREACH (pg, &obj->list, obj.list) {
    vm_page_t *new_pg = vm_page_alloc(1);
    pmap_copy_page(pg, new_pg);
    vm_object_add_page(new_obj, pg->offset, new_pg);
  }

  return new_obj;
}

void vm_map_object_dump(vm_object_t *obj) {
  WITH_MTX_LOCK (&obj->mtx) {
    vm_page_t *pg;

    TAILQ_FOREACH (pg, &obj->list, obj.list) {
      klog("(vm-obj) offset: 0x%08lx, size: %ld", pg->offset, pg->size);
    }
  }
}
