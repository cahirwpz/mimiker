#include <sys/errno.h>
#include <sys/time.h>

int do_clock_gettime(clockid_t clk, timespec_t *tp) {
  switch (clk) {
    case CLOCK_REALTIME:
      *tp = nanotime();
      break;
    case CLOCK_MONOTONIC:
      *tp = nanouptime();
      break;
    default:
      return EINVAL;
  }

  return 0;
}

int do_clock_nanosleep(clockid_t clk, int flags, const timespec_t *rqtp,
                       timespec_t *rmtp) {
  return ENOTSUP;
}
