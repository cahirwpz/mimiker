#include <sys/errno.h>
#include <sys/time.h>
#include <limits.h>

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

systime_t ts2hz(timespec_t *ts) {
  int64_t sec = ts->tv_sec;
  int ticks;

  if (sec < INT_MAX / CLK_TCK) {
    ticks = sec * CLK_TCK;
    if (ticks <= INT_MAX - ts->tv_nsec / (1000000000 / CLK_TCK))
      ticks += ts->tv_nsec / (1000000000 / CLK_TCK);
    else
      ticks = INT_MAX;
  } else
    ticks = INT_MAX;

  return ticks;
}

static int ts2timo(clockid_t clock_id, int flags, timespec_t *ts,
                   systime_t *timo, timespec_t *start) {
  int error;
  *timo = 0;

  if (ts->tv_nsec < 0 || ts->tv_nsec >= 1000000000L || ts->tv_sec < 0 ||
      (flags & ~TIMER_ABSTIME) != 0)
    return EINVAL;

  if ((error = do_clock_gettime(clock_id, start)) != 0)
    return error;

  if (flags & TIMER_ABSTIME)
    timespecsub(ts, start, ts);

  if ((ts->tv_sec == 0 && ts->tv_nsec == 0) || ts->tv_sec < 0)
    return ETIMEDOUT;

  /* Resolution of tick is lower then gettime, adding 1 tick ensure that we will
   * wait at least given time */
  *timo = ts2hz(ts) + 1;

  return 0;
}

int do_clock_nanosleep(clockid_t clk, int flags, const timespec_t *rqtp,
                       timespec_t *rmtp) {
  return ENOTSUP;
}
