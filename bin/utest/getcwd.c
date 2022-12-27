#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <errno.h>

int test_getcwd(void) {
  {
    /* Working directory is set to root if not changed */
    char buffer[256];

    assert(getcwd(buffer, 256) != NULL);
    assert(strcmp("/", buffer) == 0);
  }

  {
    /* Can change directory with absolute path */
    char buffer[256];

    assert(chdir("/") == 0);
    assert(getcwd(buffer, 256) != NULL);
    assert(strcmp("/", buffer) == 0);

    assert(chdir("/bin") == 0);
    assert(getcwd(buffer, 256) != NULL);
    assert(strcmp("/bin", buffer) == 0);

    assert(chdir("/dev/") == 0);
    assert(getcwd(buffer, 256) != NULL);
    assert(strcmp("/dev", buffer) == 0);
  }

  {
    /* Getcwd returns correct error if buffer is too short */
    char buffer[5];

    assert(chdir("/") == 0);
    assert(getcwd(buffer, 2) != NULL);
    assert(strcmp("/", buffer) == 0);

    assert(getcwd(buffer, 1) == NULL);
    assert(errno == ERANGE);

    assert(getcwd(buffer, 0) == NULL);
    assert(errno == EINVAL);

    assert(chdir("/dev") == 0);
    assert(getcwd(buffer, 5) != NULL);
    assert(strcmp("/dev", buffer) == 0);

    assert(getcwd(buffer, 4) == NULL);
    assert(errno == ERANGE);

    assert(getcwd(buffer, 0) == NULL);
    assert(errno == EINVAL);
  }

  {
    /* Can't change to invalid path */
    char buffer[256];

    assert(chdir("/") == 0);
    assert(chdir("/directory/that/doesnt/exist") == -1);
    assert(errno == ENOENT);
    assert(getcwd(buffer, 256) != NULL);
    assert(strcmp("/", buffer) == 0);
  }

  return 0;
}
