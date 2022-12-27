#include <unistd.h>

uid_t geteuid(void) {
  uid_t euid;
  getresuid(NULL, &euid, NULL);
  return euid;
}
