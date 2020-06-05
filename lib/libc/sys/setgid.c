#include <unistd.h>

int setgid(gid_t gid) {
  return setresgid(gid, -1, -1);
}
