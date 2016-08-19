#include <malloc.h>
#include <vm_object.h>
#include <libkern.h>

static inline int vm_page_cmp(vm_page_t *a, vm_page_t *b) {
  if (a->vm_offset < b->vm_offset)
    return -1;
  return a->vm_offset - b->vm_offset;
}

RB_PROTOTYPE_STATIC(vm_object_tree, vm_page, obj.tree, vm_page_cmp);
RB_GENERATE(vm_object_tree, vm_page, obj.tree, vm_page_cmp);

static MALLOC_DEFINE(mpool, "vm_object memory pool");

void vm_object_init() {
  vm_page_t *pg = pm_alloc(2);
  kmalloc_init(mpool);
  kmalloc_add_arena(mpool, pg->vaddr, PG_SIZE(pg));
}

vm_object_t *vm_object_alloc() {
  vm_object_t *obj = kmalloc(mpool, sizeof(vm_object_t), 0);
  TAILQ_INIT(&obj->list);
  RB_INIT(&obj->tree);
  return obj;
}

void vm_object_free(vm_object_t *obj) {
  while (!TAILQ_EMPTY(&obj->list)) {
    vm_page_t *pg = TAILQ_FIRST(&obj->list);
    TAILQ_REMOVE(&obj->list, pg, obj.list);
    pm_free(pg);
  }
  kfree(mpool, obj);
}

vm_page_t *vm_object_find_page(vm_object_t *obj, vm_addr_t offset) {
  vm_page_t find = {.vm_offset = offset};
  return RB_FIND(vm_object_tree, &obj->tree, &find);
}

bool vm_object_add_page(vm_object_t *obj, vm_page_t *page) {
  assert(is_aligned(page->vm_offset, PAGESIZE));
  /* For simplicity of implementation let's insert pages of size 1 only */
  assert(page->size == 1);

  if (!RB_INSERT(vm_object_tree, &obj->tree, page)) {
    obj->npages++;
    vm_page_t *next = RB_NEXT(vm_object_tree, &obj->tree, page);
    if (next)
      TAILQ_INSERT_BEFORE(next, page, obj.list);
    else
      TAILQ_INSERT_TAIL(&obj->list, page, obj.list);
    return true;
  }

  return false;
}

void vm_object_remove_page(vm_object_t *obj, vm_page_t *page) {
  TAILQ_REMOVE(&obj->list, page, obj.list);
  RB_REMOVE(vm_object_tree, &obj->tree, page);
  pm_free(page);
  obj->npages--;
}

void vm_map_object_dump(vm_object_t *obj) {
  vm_page_t *it;
  RB_FOREACH (it, vm_object_tree, &obj->tree)
    kprintf("[vm_object] offset: %lu, size: %lu \n", it->vm_offset, it->size);
}
