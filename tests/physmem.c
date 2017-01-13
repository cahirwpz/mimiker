#include <stdc.h>
#include <physmem.h>
#include <test.h>

unsigned long pm_hash();

int test_physmem() {
  unsigned long pre = pm_hash();

  /* Write - read test */
  vm_page_t *pg = pm_alloc(16);
  int size = PAGESIZE * 16;
  char *arr = (char *)pg->vaddr;
  for (int i = 0; i < size; i++)
    arr[i] = 42; /* Write non-zero value */
  for (int i = 0; i < size; i++)
    assert(arr[i] == 42);

  /* Allocate deallocate test */
  const int N = 7;
  vm_page_t *pgs[N];
  for (int i = 0; i < N; i++)
    pgs[i] = pm_alloc(1 << i);
  for (int i = 0; i < N; i += 2)
    pm_free(pgs[i]);
  for (int i = 1; i < N; i += 2)
    pm_free(pgs[i]);

  assert(pre != pm_hash());
  pm_free(pg);
  assert(pre == pm_hash());

  pre = pm_hash();
  vm_page_t *pg1 = pm_alloc(4);
  vm_page_t *pg3 = pm_split_alloc_page(pg1);
  vm_page_t *pg2 = pm_split_alloc_page(pg1);
  vm_page_t *pg4 = pm_split_alloc_page(pg3);

  pm_free(pg3);
  assert(pre != pm_hash());
  pm_free(pg4);
  assert(pre != pm_hash());
  pm_free(pg2);
  assert(pre != pm_hash());
  pm_free(pg1);
  assert(pre == pm_hash());
  kprintf("Tests passed\n");
  return 0;
}

TEST_ADD(physmem, test_physmem);
