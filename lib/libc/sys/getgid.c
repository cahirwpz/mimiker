#include <unistd.h>

gid_t getgid(void) {
  gid_t gid;
  getresgid(&gid, NULL, NULL);
  return gid;
}
