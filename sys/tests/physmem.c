#include <sys/mimiker.h>
#include <sys/libkern.h>
#include <sys/vm_physmem.h>
#include <sys/ktest.h>

static int test_physmem(void) {
  const int N = 7;
  vm_page_t *pgs[N];
  for (int i = 0; i < N; i++)
    pgs[i] = vm_page_alloc(1 << i);
  for (int i = 0; i < N; i += 2)
    vm_page_free(pgs[i]);
  for (int i = 1; i < N; i += 2)
    vm_page_free(pgs[i]);

  return KTEST_SUCCESS;
}

KTEST_ADD(physmem, test_physmem, 0);
