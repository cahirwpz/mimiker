#define KL_LOG KL_VM
#include <sys/klog.h>
#include <sys/mimiker.h>
#include <sys/pool.h>
#include <sys/pmap.h>
#include <sys/uvm_object.h>
#include <sys/vm_physmem.h>

static POOL_DEFINE(P_VMOBJ, "uvm_object", sizeof(uvm_object_t));

uvm_object_t *uvm_object_alloc(vm_pgr_type_t type) {
  uvm_object_t *obj = pool_alloc(P_VMOBJ, M_ZERO);
  TAILQ_INIT(&obj->uo_pages);
  mtx_init(&obj->uo_lock, 0);
  obj->uo_pager = &pagers[type];
  obj->uo_refs = 1;
  return obj;
}

vm_page_t *uvm_object_find_page(uvm_object_t *obj, vm_offset_t offset) {
  SCOPED_MTX_LOCK(&obj->uo_lock);

  vm_page_t *pg;
  TAILQ_FOREACH (pg, &obj->uo_pages, objpages) {
    if (pg->offset == offset)
      return pg;
  }

  return NULL;
}

void uvm_object_add_page(uvm_object_t *obj, vm_offset_t offset, vm_page_t *pg) {
  assert(page_aligned_p(pg->offset));
  /* For simplicity of implementation let's insert pages of size 1 only */
  assert(pg->size == 1);

  pg->object = obj;
  pg->offset = offset;

  WITH_MTX_LOCK (&obj->uo_lock) {
    vm_page_t *it;
    TAILQ_FOREACH (it, &obj->uo_pages, objpages) {
      if (it->offset > pg->offset) {
        TAILQ_INSERT_BEFORE(it, pg, objpages);
        obj->uo_npages++;
        return;
      }
      /* there must be no page at the offset! */
      assert(it->offset != pg->offset);
    }

    /* offset of page is greater than the offset of any other page */
    TAILQ_INSERT_TAIL(&obj->uo_pages, pg, objpages);
    obj->uo_npages++;
  }
}

static void uvm_object_remove_pages_nolock(uvm_object_t *obj,
                                           vm_offset_t offset, size_t length) {
  assert(mtx_owned(&obj->uo_lock));
  assert(page_aligned_p(offset) && page_aligned_p(length));

  vm_page_t *pg, *next;
  TAILQ_FOREACH_SAFE (pg, &obj->uo_pages, objpages, next) {
    if (pg->offset >= offset + length)
      break;
    if (pg->offset < offset)
      continue;

    pg->offset = 0;
    pg->object = NULL;
    TAILQ_REMOVE(&obj->uo_pages, pg, objpages);
    vm_page_free(pg);
    obj->uo_npages--;
  }
}

void uvm_object_remove_pages(uvm_object_t *obj, vm_offset_t off, size_t len) {
  SCOPED_MTX_LOCK(&obj->uo_lock);
  uvm_object_remove_pages_nolock(obj, off, len);
}

#define uvm_object_remove_all_pages(obj)                                       \
  uvm_object_remove_pages_nolock((obj), 0, (size_t)(-PAGESIZE))

void uvm_object_hold(uvm_object_t *obj) {
  refcnt_acquire(&obj->uo_refs);
}

void uvm_object_drop(uvm_object_t *obj) {
  WITH_MTX_LOCK (&obj->uo_lock) {
    if (!refcnt_release(&obj->uo_refs))
      return;

    uvm_object_remove_all_pages(obj);
  }
  pool_free(P_VMOBJ, obj);
}

uvm_object_t *uvm_object_clone(uvm_object_t *obj) {
  /* XXX: this function will not be used in UVM */
  uvm_object_t *new_obj = uvm_object_alloc(VM_DUMMY);
  new_obj->uo_pager = obj->uo_pager;

  WITH_MTX_LOCK (&obj->uo_lock) {
    vm_page_t *pg;
    TAILQ_FOREACH (pg, &obj->uo_pages, objpages) {
      vm_page_t *new_pg = vm_page_alloc(1);
      pmap_copy_page(pg, new_pg);
      uvm_object_add_page(new_obj, pg->offset, new_pg);
    }
  }

  return new_obj;
}

void vm_map_object_dump(uvm_object_t *obj) {
  SCOPED_MTX_LOCK(&obj->uo_lock);

  vm_page_t *pg;
  TAILQ_FOREACH (pg, &obj->uo_pages, objpages) {
    klog("(uvm-obj) offset: 0x%08lx, size: %ld", pg->offset, pg->size);
  }
}
