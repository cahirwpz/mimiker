#include <time.h>
#include <errno.h>
#include <thread.h>

int do_clock_gettime(clockid_t clk, timespec_t *tp) {
  if (!tp) {
    errno = EFAULT;
    return -1;
  }
  switch (clk) {
    case CLOCK_MONOTONIC: {
      *tp = tv2ts(get_uptime());
      break;
    }
    case CLOCK_REALTIME:
      break; /* TODO Need RTC */
    default: {
      errno = EINVAL;
      return -1;
    }
  }
  return 0;
}

int do_nanosleep(timespec_t *rqtp, timespec_t *rmtp) {
  return 0;
}

int do_gettimeofday(timeval_t *tp, void *tzp) {
  if (tp)
    *tp = get_uptime();
  return 0;
}
