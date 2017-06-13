#include <errno.h>
#include <thread.h>
#include <time.h>
#include <timer.h>
#include <sync.h>

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

static void waker(timer_event_t *tev) {
  sleepq_signal(tev);
}

int do_clock_nanosleep(clockid_t clk, int flags, const timespec_t *rqtp,
                       timespec_t *rmtp) {
  if (rqtp == NULL || rqtp->tv_nsec < 0 || rqtp->tv_nsec > 1000000000) {
    errno = EINVAL;
    return -1;
  }
  timeval_t tv =
    (timeval_t){.tv_sec = rqtp->tv_sec, .tv_usec = rqtp->tv_nsec / 1000};
  timer_event_t tev;
  tev.tev_when = get_uptime();
  tev.tev_when = timeval_add(&tev.tev_when, &tv);
  tev.tev_func = waker;
  CRITICAL_SECTION {
    cpu_timer_add_event(&tev);
    sleepq_wait(&tev, "nanosleep");
  }
  return 0;
}
