#include "utest.h"
#include <sys/mman.h>

void xmprotect(void *addr, size_t len, int prot) {
  int result = mprotect(addr, len, prot);

  if (result < 0)
    die("mprotect: %s", strerror(errno));
}
