#include <sys/mutex.h>
#include <sys/signal.h>
#include <sys/errno.h>
#include <sys/sleepq.h>
#include <sys/time.h>
#include <sys/callout.h>
#include <sys/proc.h>
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

static systime_t ts2hz(const timespec_t *ts) {
  if (ts->tv_sec < 0 || (ts->tv_sec == 0 && ts->tv_nsec == 0))
    return 0;

  if (ts->tv_sec > UINT_MAX / CLK_TCK)
    return UINT_MAX;

  const int pow9 = 1000000000;
  int tick = pow9 / CLK_TCK;

  /* Round up the number of ticks */
  long nsectck = (ts->tv_nsec + tick - 1) / tick;
  systime_t ticks = ts->tv_sec * CLK_TCK;

  if (ticks > UINT_MAX - nsectck - 1)
    return UINT_MAX;

  /* Add 1 for the current tick to expire */
  return ticks + nsectck + 1;
}

static bool timespec_is_canonical(const timespec_t *ts) {
  if (ts->tv_nsec < 0 || ts->tv_nsec >= 1000000000L || ts->tv_sec < 0)
    return false;
  return true;
}

static bool timeval_is_canonical(const timeval_t *tv) {
  if (tv->tv_usec < 0 || tv->tv_usec >= 1000000 || tv->tv_sec < 0)
    return false;
  return true;
}

static int ts2timo(clockid_t clock_id, int flags, timespec_t *ts,
                   systime_t *timo, timespec_t *start) {
  int error;
  *timo = 0;

  if (!timespec_is_canonical(ts) || (flags & ~TIMER_ABSTIME))
    return EINVAL;

  if ((error = do_clock_gettime(clock_id, start)))
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
  /* rmt - remaining time, rqt - requested time, p - pointer */
  timespec_t rmt_start, rmt_end, rmt;
  systime_t timo;
  int error, error2;

  if ((error = ts2timo(clk, flags, rqtp, &timo, &rmt_start))) {
    if (error == ETIMEDOUT)
      goto timedout;
    return error;
  }

  do {
    error = sleepq_wait_timed((void *)(&rmt_start), __caller(0), timo);
    if (error == ETIMEDOUT)
      goto timedout;

    if ((error2 = do_clock_gettime(clk, &rmt_end)))
      return error2;

    if (flags == TIMER_ABSTIME) {
      timespecsub(rqtp, &rmt_end, &rmt);
    } else {
      timespecsub(&rmt_end, &rmt_start, &rmt);
      timespecsub(rqtp, &rmt, &rmt);
    }
    if (rmt.tv_sec < 0)
      timespecclear(&rmt);

    if (rmtp)
      *rmtp = rmt;
    if (error)
      return error;

    timo = ts2hz(&rmt);
  } while (timo > 0);

  return 0;

timedout:
  if (rmtp)
    rmtp->tv_sec = rmtp->tv_nsec = 0;
  return 0;
}

time_t tm2sec(tm_t *t) {
  if (t->tm_year < 70)
    return 0;

  const int32_t year_scale_s = 31536000, day_scale_s = 86400,
                hour_scale_s = 3600, min_scale_s = 60;
  time_t res = 0;
  static const int month_in_days[13] = {0,   31,  59,  90,  120, 151,
                                        181, 212, 243, 273, 304, 334};

  res += (time_t)month_in_days[t->tm_mon] * day_scale_s;
  /* Extra days from leap years which already past (EPOCH) */
  res += (time_t)((t->tm_year + 1900) / 4 - (1970) / 4) * day_scale_s;
  res -= (time_t)((t->tm_year + 1900) / 100 - (1970) / 100) * day_scale_s;
  res += (time_t)((t->tm_year + 1900) / 400 - (1970) / 400) * day_scale_s;

  /* If actual year is a leap year and the leap day passed */
  if (t->tm_mon > 1 && (t->tm_year % 4) == 0 &&
      ((t->tm_year % 100) != 0 || (t->tm_year + 1900) % 400) == 0)
    res += day_scale_s;

  /* (t.tm_mday - 1) - cause days are in range [1-31] */
  return (time_t)(t->tm_year - 70) * year_scale_s +
         (t->tm_mday - 1) * day_scale_s + t->tm_hour * hour_scale_s +
         t->tm_min * min_scale_s + t->tm_sec + res;
}

static void hz2tv(systime_t hz, timeval_t *tv) {
  const uint32_t us_per_sec = 1000000;
  const uint32_t us_per_tick = us_per_sec / CLK_TCK;
  tv->tv_sec = hz / CLK_TCK;
  tv->tv_usec = (hz - (tv->tv_sec * CLK_TCK)) * us_per_tick;
}

static systime_t tv2hz(const timeval_t *tv) {
  timespec_t ts = {.tv_sec = tv->tv_sec, .tv_nsec = (long)tv->tv_usec * 1000};
  return ts2hz(&ts);
}

static void kitimer_get(proc_t *p, kitimer_t *timer, struct itimerval *tval) {
  assert(mtx_owned(&p->p_lock));

  if (p->p_flags & PF_ITIMER_ACTIVE) {
    /* it_value is the time until next timer tick. */
    hz2tv(p->p_itimer.kit_callout.c_time - getsystime(), &tval->it_value);
    hz2tv(p->p_itimer.kit_interval, &tval->it_interval);
  } else {
    *tval = (struct itimerval){0};
  }
}

int do_getitimer(proc_t *p, int which, struct itimerval *tval) {
  if (which == ITIMER_PROF || which == ITIMER_VIRTUAL)
    return ENOTSUP;
  else if (which != ITIMER_REAL)
    return EINVAL;

  SCOPED_MTX_LOCK(&p->p_lock);

  kitimer_get(p, &p->p_itimer, tval);

  return 0;
}

void kitimer_stop(proc_t *p, kitimer_t *timer) {
  assert(mtx_owned(&p->p_lock));

  if (p->p_flags & PF_ITIMER_ACTIVE) {
    /* The callout is pending or active.
     * If it has already been delegated to the callout thread we must
     * go to sleep waiting for its completion, so we release the mutex
     * before sleeping in callout_drain(). */
    if (!callout_stop(&timer->kit_callout)) {
      mtx_unlock(&p->p_lock);
      callout_drain(&timer->kit_callout);
      mtx_lock(&p->p_lock);
      /* The callout should have cleared the flag. */
      assert(!(p->p_flags & PF_ITIMER_ACTIVE));
      return;
    }
    p->p_flags &= ~PF_ITIMER_ACTIVE;
  }
}

static void kitimer_timeout(void *arg) {
  proc_t *p = arg;

  SCOPED_MTX_LOCK(&p->p_lock);

  assert(p->p_flags & PF_ITIMER_ACTIVE);

  if (!proc_is_alive(p)) {
    p->p_flags &= ~PF_ITIMER_ACTIVE;
    return;
  }

  sig_kill(p, &DEF_KSI_RAW(SIGALRM));

  if (p->p_itimer.kit_callout.c_interval == 0)
    p->p_flags &= ~PF_ITIMER_ACTIVE;
}

/* The timer must have been stopped prior to calling this function. */
static void kitimer_setup(proc_t *p, kitimer_t *timer, systime_t next,
                          systime_t interval) {
  assert(mtx_owned(&p->p_lock));
  assert(!(p->p_flags & PF_ITIMER_ACTIVE));

  if (next) {
    timer->kit_interval = interval;
    p->p_flags |= PF_ITIMER_ACTIVE;
    callout_setup_relative_periodic(&timer->kit_callout, next, interval,
                                    kitimer_timeout, p);
  }
}

int do_setitimer(proc_t *p, int which, const struct itimerval *itval,
                 struct itimerval *oval) {
  if (which == ITIMER_PROF || which == ITIMER_VIRTUAL)
    return ENOTSUP;
  else if (which != ITIMER_REAL)
    return EINVAL;

  if (!timeval_is_canonical(&itval->it_value) ||
      !timeval_is_canonical(&itval->it_interval))
    return EINVAL;

  systime_t next = tv2hz(&itval->it_value);
  systime_t interval =
    tv2hz(&itval->it_interval) - 1; /* ts2hz increments its result by 1 */

  if (next == UINT_MAX || interval == UINT_MAX)
    return EINVAL;

  SCOPED_MTX_LOCK(&p->p_lock);

  kitimer_t *timer = &p->p_itimer;

  if (oval) {
    /* Store old timer value. */
    kitimer_get(p, timer, oval);
  }

  kitimer_stop(p, timer);
  kitimer_setup(p, timer, next, interval);

  return 0;
}
