#include <unistd.h>

gid_t getegid() {
  gid_t egid;
  getresgid(NULL, &egid, NULL);
  return egid;
}
