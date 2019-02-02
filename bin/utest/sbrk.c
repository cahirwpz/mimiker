#include "utest.h"

#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>

static void *sbrk_orig = NULL;

static void sbrk_good(void) {
  /* Initial sbrk */
  char *a1 = sbrk(10);
  /* Test write access. */
  memset(a1, 1, 10);
  /* Expand sbrk a little bit. */
  char *a2 = sbrk(40);
  assert(a2 == a1 + 10);
  /* And again. */
  char *a3 = sbrk(50);
  assert(a3 == a2 + 40);
  /* Now expand it a lot, much more than page size. */
  char *a4 = sbrk(0x5000);
  assert(a4 == a3 + 50);
  /* Test write access. */
  memset(a4, -1, 0x5000);
  /* See that previous data is unmodified. */
  assert(*(a1 + 5) == 1);
}

static void sbrk_bad(void) {
  void *sbrk_now = sbrk(0);
  /* Attempt to move sbrk before original start. */
  sbrk(sbrk_orig - sbrk_now - 0x10000);
  assert(errno == EINVAL);
#if 1
  /* Now, try shrinking data. */
  sbrk(-1 * (0x5000 + 50 + 40 + 10));
  assert(errno == EINVAL);
  /* Get new brk end */
  char *a5 = sbrk(0);
  assert(sbrk_orig == a5);
#else
  /* Note: sbrk shrinking not yet implemented! */
  sbrk(4096);
  sbrk(-4096);
  assert(errno == ENOTSUP);
#endif
}

int test_sbrk() {
  sbrk_orig = sbrk(0);
  assert(sbrk_orig != NULL);

  sbrk_bad();
  sbrk_good();
  return 0;
}
