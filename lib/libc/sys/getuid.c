#include <unistd.h>

uid_t getuid() {
  uid_t uid;
  getresuid(&uid, NULL, NULL);
  return uid;
}
