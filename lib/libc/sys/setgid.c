#include <unistd.h>

int setgid(gid_t gid) {
  return setregid(gid, -1, -1);
}
