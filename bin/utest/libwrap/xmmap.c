#include "utest.h"
#include <sys/mman.h>

void *xmmap(void *addr, size_t len, int prot, int flags, int fd, off_t offset) {
  void *result = mmap(addr, len, prot, flags, fd, offset);

  if (result == MAP_FAILED)
    die("mmap: %s", strerror(errno));

  return result;
}
