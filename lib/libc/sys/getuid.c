#include <unistd.h>

uid_t getuid(void) {
  uid_t uid;
  getresuid(&uid, NULL, NULL);
  return uid;
}
