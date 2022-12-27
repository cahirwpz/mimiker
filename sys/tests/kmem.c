#include <sys/klog.h>
#include <sys/ktest.h>
#include <sys/kmem.h>
#include <sys/vm.h>

#define MEMSZ (PAGESIZE * 255)

static int test_kmem(void) {
  /* Write - read test */
  volatile unsigned *arr = kmem_alloc(MEMSZ, 0);
  assert(arr != NULL);
  int size = MEMSZ / sizeof(int);
  for (int i = 0; i < size; i++)
    arr[i] = 0xDEADC0DE; /* Write non-zero value */
  for (int i = 0; i < size; i++)
    assert(arr[i] == 0xDEADC0DE);
  kmem_free((void *)arr, MEMSZ);

  return KTEST_SUCCESS;
}

KTEST_ADD(kmem, test_kmem, 0);
