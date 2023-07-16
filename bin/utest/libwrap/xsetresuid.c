#include "utest.h"
#include <unistd.h>

void xsetresuid(uid_t ruid, uid_t euid, uid_t suid) {
  int result = setresuid(ruid, euid, suid);

  if (result < 0)
    die("setresuid: %s", strerror(errno));
}
