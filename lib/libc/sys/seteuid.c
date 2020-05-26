#include <unistd.h>

int seteuid(uid_t euid) {
  return setresuid(-1, euid, -1);
}
