#include "utest.h"
#include <sys/mman.h>

void xmunmap(void *addr, size_t length) {
  int result = munmap(addr, length);

  if (result < 0)
    die("munmap: %s", strerror(errno));
}
