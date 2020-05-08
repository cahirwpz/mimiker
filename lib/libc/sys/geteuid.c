#include <unistd.h>

uid_t geteuid() {
  uid_t euid;
  getresuid(NULL, &euid, NULL);
  return euid;
}
