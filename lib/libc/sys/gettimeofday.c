#include <sys/time.h>

int gettimeofday(timeval_t *tp, void *tzp) {
  if (tp) {
    timespec_t ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    *tp = ts2tv(ts);
  }
  return 0;
}
