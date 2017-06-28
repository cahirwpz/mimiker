#include "utest.h"

#include <string.h>
#include <unistd.h>
#include <assert.h>

int test_sbrk() {
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
#if 0
  /* Note: sbrk shrinking not yet implemented! */
  /* Now, try shrinking data. */
  sbrk(-1 * (0x5000 + 50 + 40 + 10));
  /* Get new brk end */
  char *a5 = sbrk(0);
  assert(a1 == a5);
#endif
  return 0;
}
