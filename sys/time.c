#include <time.h>

int do_clock_gettime(clockid_t clk, timespec_t *tp) {
  return 0;
}

int do_nanosleep(timespec_t *rqtp, timespec_t *rmtp) {
  return 0;
}

int do_gettimeofday(timeval_t *tp, void *tzp) {
  return 0;
}
