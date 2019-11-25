
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

int test_getcwd(void) {
  char buffer[256];
  getcwd(buffer, 256);
  assert(strcmp("/", buffer) == 0);

  if (chdir("/bin") == -1) {
    return 1;
  }

  getcwd(buffer, 256);
  assert(strcmp("/bin", buffer) == 0);

  if (chdir("/dev/") == -1) {
    return 1;
  }

  getcwd(buffer, 256);
  assert(strcmp("/dev", buffer) == 0);
  return 0;
}
