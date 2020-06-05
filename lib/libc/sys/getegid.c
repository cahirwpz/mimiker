#include <unistd.h>

gid_t getegid(void) {
  gid_t egid;
  getresgid(NULL, &egid, NULL);
  return egid;
}
