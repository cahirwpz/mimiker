#include <sys/errno.h>
#include <sys/time.h>

int do_clock_gettime(clockid_t clk, timespec_t *tp) {
  bintime_t bin;
  switch (clk) {
    case CLOCK_REALTIME:
      bin = bintime();
      break;
    case CLOCK_MONOTONIC:
      bin = binuptime();
      break;
    default:
      return EINVAL;
  }
  bt2ts(&bin, tp);
  return 0;
}

int do_clock_nanosleep(clockid_t clk, int flags, const timespec_t *rqtp,
                       timespec_t *rmtp) {
  return ENOTSUP;
}
