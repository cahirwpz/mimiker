#include "utest.h"
#include <unistd.h>

void xgetresuid(uid_t *ruid, uid_t *euid, uid_t *suid) {
  int result = getresuid(ruid, euid, suid);

  if (result < 0)
    die("getresuid: %s", strerror(errno));
}
