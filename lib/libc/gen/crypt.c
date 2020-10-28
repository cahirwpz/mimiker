#include <unistd.h>
#include <string.h>

const char *crypt(const char *key, const char *salt) {
  int max_pass = sysconf(_SC_PASS_MAX);
  return strndup(key, max_pass);
}
