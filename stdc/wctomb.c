#include <stddef.h>

int wctomb(char *s, wchar_t wchar __attribute__((unused))) {
  return (s == NULL) ? 0 : -1;
}
