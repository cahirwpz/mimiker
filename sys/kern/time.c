#include <sys/errno.h>
#include <sys/sleepq.h>
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
  systime_t ticks;

  if (ts->tv_sec < 0 || (ts->tv_sec == 0 && ts->tv_nsec == 0))
    return 0;

  if (ts->tv_sec <= UINT_MAX / CLK_TCK) {
    const int pow9 = 1000000000;
    int tick = pow9 / CLK_TCK;
    /* We are rounding up the number of ticks */
    long nsectck = (ts->tv_nsec + tick - 1) / tick;
    ticks = ts->tv_sec * CLK_TCK;

    if (ticks <= UINT_MAX - nsectck - 1) {
      /* We are adding 1 for the current tick to expire */
      ticks += nsectck + 1;
    } else
      ticks = UINT_MAX;
  } else
    ticks = UINT_MAX;

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

  *timo = ts2hz(ts);

  return 0;
}

int do_clock_nanosleep(clockid_t clk, int flags, timespec_t *rqtp,
                       timespec_t *rmtp) {
  timespec_t rmtstart;
  int error;
  systime_t timo;

  if ((error = ts2timo(clk, flags, rqtp, &timo, &rmtstart)) != 0) {
    if (error == ETIMEDOUT) {
      error = 0;
      if (rmtp != NULL)
        rmtp->tv_sec = rmtp->tv_nsec = 0;
    }
    return error;
  }

again:
  error = sleepq_wait_timed((void *)(&rmtstart), __caller(0), timo);

  if (error == ETIMEDOUT) {
    if (rmtp != NULL)
      rmtp->tv_sec = rmtp->tv_nsec = 0;
    return 0;
  }

  if (rmtp != NULL || error == 0) {
    timespec_t rmtend, tsvar;
    timespec_t *lefttp = rmtp ? rmtp : &tsvar;
    int err;

    err = do_clock_gettime(clk, &rmtend);
    if (err != 0)
      return err;

    if (flags == TIMER_ABSTIME) {
      timespecsub(rqtp, &rmtend, lefttp);
    } else {
      timespecsub(&rmtend, &rmtstart, lefttp);
      timespecsub(rqtp, lefttp, lefttp);
    }
    if (lefttp->tv_sec < 0)
      timespecclear(lefttp);
    if (error == 0) {
      timo = ts2hz(lefttp);
      if (timo > 0)
        goto again;
    }
  }

  return error;
}
