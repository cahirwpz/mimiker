#include <time.h>

int nanosleep(timespec_t *rqtp, timespec_t *rmtp) {
  return do_clock_nanosleep(CLOCK_REALTIME, 0, rqtp, rmtp);
}

int do_gettimeofday(timeval_t *tp, void *tzp) {
  if (tp) {
    timespec_t ts;
    do_clock_gettime(CLOCK_REALTIME, &ts);
    *tp = ts2tv(ts);
  }
  return 0;
}
