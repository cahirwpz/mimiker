#include <ktest.h>
#include <time.h>

suseconds_t sleep_times[] = {1, 12, 123, 1234, 12345, 123456, 1234567};

static int test_sleep(time_t sec, long nsec, suseconds_t min_us_diff) {
  timespec_t ts = (timespec_t){.tv_sec = sec, .tv_nsec = nsec};
  timespec_t start_ts, end_ts;
  int result = do_clock_gettime(CLOCK_MONOTONIC, &start_ts);
  if (result != 0)
    return result;
  result = do_clock_nanosleep(CLOCK_MONOTONIC, 0, &ts, NULL);
  if (result != 0)
    return result;
  result = do_clock_gettime(CLOCK_MONOTONIC, &end_ts);
  if (result != 0)
    return result;
  timeval_t start_tv = ts2tv(start_ts);
  timeval_t end_tv = ts2tv(end_ts);
  timeval_t diff = timeval_sub(&end_tv, &start_tv);
  if (diff.tv_usec + (suseconds_t)diff.tv_sec * 1000000 < min_us_diff)
    return 1;
  return 0;
}

static int time_syscalls(void) {
  const int n = sizeof(sleep_times) / sizeof(sleep_times[0]);
  for (int i = 0; i < n; i++) {
    time_t sec = sleep_times[i] / 1000000;
    long nsec = (sleep_times[i] % 1000000) * 1000;
    int result = test_sleep(sec, nsec, sleep_times[i]);
    assert(result == 0);
  }
  return 0;
}

KTEST_ADD(time_syscalls, time_syscalls, 0);
