#include <stdc.h>
#include <malloc.h>

int main() {
  vm_page_t *page = pm_alloc(1);

  MALLOC_DEFINE(mp, "testing memory pool");

  kmalloc_init(mp);
  kmalloc_add_arena(mp, page->vaddr, PAGESIZE);

  void *ptr1 = kmalloc(mp, 15, 0);
  assert(ptr1 != NULL);

  void *ptr2 = kmalloc(mp, 23, 0);
  assert(ptr2 != NULL && ptr2 > ptr1);

  void *ptr3 = kmalloc(mp, 7, 0);
  assert(ptr3 != NULL && ptr3 > ptr2);

  void *ptr4 = kmalloc(mp, 2000, 0);
  assert(ptr4 != NULL && ptr4 > ptr3);

  void *ptr5 = kmalloc(mp, 1000, 0);
  assert(ptr5 != NULL);

  kfree(mp, ptr1);
  kfree(mp, ptr2);
  kmalloc_dump(mp);
  kfree(mp, ptr3);
  kfree(mp, ptr5);

  void *ptr6 = kmalloc(mp, 2000, M_NOWAIT);
  assert(ptr6 == NULL);

  pm_free(page);

  return 0;
}
