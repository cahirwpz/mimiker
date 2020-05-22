#include <unistd.h>

int setegid(gid_t egid) {
  return setresgid(-1, egid, -1);
}
