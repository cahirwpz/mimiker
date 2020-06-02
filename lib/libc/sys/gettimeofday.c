#include <sys/time.h>

int gettimeofday(timeval_t *tp, void *tzp) {
  if (tp) {
    timespec_t ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    tp->tv_sec = ts.tv_sec;
    tp->tv_usec = ts.tv_nsec / 1000;
  }
  return 0;
}
