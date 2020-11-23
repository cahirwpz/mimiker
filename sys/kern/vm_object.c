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

RB_PROTOTYPE_STATIC(vm_pagetree, vm_page, obj.tree, vm_page_cmp);
RB_GENERATE(vm_pagetree, vm_page, obj.tree, vm_page_cmp);

vm_object_t *vm_object_alloc(vm_pgr_type_t type) {
  vm_object_t *obj = pool_alloc(P_VMOBJ, M_ZERO);
  TAILQ_INIT(&obj->list);
  RB_INIT(&obj->tree);
  mtx_init(&obj->mtx, 0);
  obj->pager = &pagers[type];
  obj->ref_counter = 1;
  return obj;
}

void vm_object_free(vm_object_t *obj) {
  WITH_MTX_LOCK (&obj->mtx) {
    if (!refcnt_release(&obj->ref_counter)) {
      return;
    }

    while (!TAILQ_EMPTY(&obj->list)) {
      vm_page_t *pg = TAILQ_FIRST(&obj->list);
      TAILQ_REMOVE(&obj->list, pg, obj.list);
      vm_page_free(pg);
    }
  }
  pool_free(P_VMOBJ, obj);
}

vm_page_t *vm_object_find_page(vm_object_t *obj, off_t offset) {
  vm_page_t find = {.offset = offset};
  SCOPED_MTX_LOCK(&obj->mtx);
  return RB_FIND(vm_pagetree, &obj->tree, &find);
}

bool vm_object_add_page(vm_object_t *obj, off_t offset, vm_page_t *page) {
  assert(page_aligned_p(page->offset));
  /* For simplicity of implementation let's insert pages of size 1 only */
  assert(page->size == 1);

  page->object = obj;
  page->offset = offset;

  SCOPED_MTX_LOCK(&obj->mtx);

  if (!RB_INSERT(vm_pagetree, &obj->tree, page)) {
    obj->npages++;
    vm_page_t *next = RB_NEXT(vm_pagetree, &obj->tree, page);
    if (next)
      TAILQ_INSERT_BEFORE(next, page, obj.list);
    else
      TAILQ_INSERT_TAIL(&obj->list, page, obj.list);
    return true;
  }

  return false;
}

static void vm_object_remove_page_nolock(vm_object_t *obj, vm_page_t *page) {
  page->offset = 0;
  page->object = NULL;

  TAILQ_REMOVE(&obj->list, page, obj.list);
  RB_REMOVE(vm_pagetree, &obj->tree, page);
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
  vm_page_t *it;

  WITH_MTX_LOCK (&obj->mtx) {
    RB_FOREACH (it, vm_pagetree, &obj->tree)
      klog("(vm-obj) offset: 0x%08lx, size: %ld", it->offset, it->size);
  }
}
