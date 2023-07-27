#include "utest.h"
#include <unistd.h>

void xgetresgid(uid_t *ruid, uid_t *euid, uid_t *suid) {
  int result = getresgid(ruid, euid, suid);

  if (result < 0)
    die("getresgid: %s", strerror(errno));
}
