#include <unistd.h>
#include <string.h>

#define PASS_MAX 128

const char *crypt(const char *key, const char *salt) {
  return strndup(key, PASS_MAX);
}
