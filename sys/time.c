#include <errno.h>
#include <thread.h>
#include <time.h>
#include <timer.h>
#include <sync.h>

int do_clock_gettime(clockid_t clk, timespec_t *tp) {
  if (tp == NULL)
    return -EFAULT;

  int error = -EINVAL;

  if (clk == CLOCK_MONOTONIC)
    *tp = tv2ts(get_uptime());

  return error;
}

static void waker(timer_event_t *tev) {
  sleepq_signal(tev);
}

int do_clock_nanosleep(clockid_t clk, int flags, const timespec_t *rqtp,
                       timespec_t *rmtp) {
  if (rqtp == NULL)
    return -EFAULT;
  if (rqtp->tv_nsec < 0 || rqtp->tv_nsec > 1000000000)
    return -EINVAL;

  timeval_t now = get_uptime();
  timeval_t diff = ts2tv(*rqtp);
  timer_event_t tev = {.tev_when = timeval_add(&now, &diff), .tev_func = waker};

  CRITICAL_SECTION {
    cpu_timer_add_event(&tev);
    sleepq_wait(&tev, "clock_nanosleep");
  }
  return 0;
}
