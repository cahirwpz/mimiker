#include <unistd.h>

int setuid(uid_t uid) {
  return setresuid(uid, -1, -1);
}
