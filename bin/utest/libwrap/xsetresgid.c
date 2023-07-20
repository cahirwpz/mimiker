#include "utest.h"
#include <unistd.h>

void xsetresgid(uid_t ruid, uid_t euid, uid_t suid) {
  int result = setresgid(ruid, euid, suid);

  if (result < 0)
    die("setresgid: %s", strerror(errno));
}
