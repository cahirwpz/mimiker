#include "utest.h"
#include <sys/time.h>

void xgettimeofday(struct timeval *tv, struct timezone *tz) {
  int result = gettimeofday(tv, tz);

  if (result < 0)
    die("gettimeofday: %s", strerror(errno));
}
