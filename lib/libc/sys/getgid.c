#include <unistd.h>

gid_t getgid() {
  gid_t gid;
  getresgid(&gid, NULL, NULL);
  return gid;
}
