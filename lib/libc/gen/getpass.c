#include <unistd.h>

char *getpass(const char *prompt) {
  static char c[1] = "\0";
  return c;
}
