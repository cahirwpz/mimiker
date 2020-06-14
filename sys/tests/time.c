#include <sys/mimiker.h>
#include <errno.h>
#include <sys/ktest.h>
#include <sys/time.h>

static int test_do_clock_nanosleep(void) {
  timespec_t rmt, rqt;
  int clk_mono = CLOCK_MONOTONIC, clk_rltm = CLOCK_REALTIME;
  /* Incorrect requested time argument */
  rqt.tv_sec = -1;
  rqt.tv_nsec = 0;
  assert(do_clock_nanosleep(clk_mono, TIMER_ABSTIME, &rmt, NULL) == EINVAL);
  assert(do_clock_nanosleep(clk_rltm, TIMER_ABSTIME, &rmt, NULL) == EINVAL);
  assert(do_clock_nanosleep(clk_mono, TIMER_ABSTIME, &rmt, &rqt) == EINVAL);
  assert(do_clock_nanosleep(clk_rltm, TIMER_ABSTIME, &rmt, &rqt) == EINVAL);
  assert(do_clock_nanosleep(clk_mono, TIMER_RELTIME, &rmt, NULL) == EINVAL);
  assert(do_clock_nanosleep(clk_rltm, TIMER_RELTIME, &rmt, NULL) == EINVAL);
  assert(do_clock_nanosleep(clk_mono, TIMER_RELTIME, &rmt, &rqt) == EINVAL);
  assert(do_clock_nanosleep(clk_rltm, TIMER_RELTIME, &rmt, &rqt) == EINVAL);
  rqt.tv_sec = 0;
  rqt.tv_nsec = -1;
  assert(do_clock_nanosleep(clk_mono, TIMER_ABSTIME, &rmt, NULL) == EINVAL);
  assert(do_clock_nanosleep(clk_rltm, TIMER_ABSTIME, &rmt, NULL) == EINVAL);
  assert(do_clock_nanosleep(clk_mono, TIMER_ABSTIME, &rmt, &rqt) == EINVAL);
  assert(do_clock_nanosleep(clk_rltm, TIMER_ABSTIME, &rmt, &rqt) == EINVAL);
  assert(do_clock_nanosleep(clk_mono, TIMER_RELTIME, &rmt, NULL) == EINVAL);
  assert(do_clock_nanosleep(clk_rltm, TIMER_RELTIME, &rmt, NULL) == EINVAL);
  assert(do_clock_nanosleep(clk_mono, TIMER_RELTIME, &rmt, &rqt) == EINVAL);
  assert(do_clock_nanosleep(clk_rltm, TIMER_RELTIME, &rmt, &rqt) == EINVAL);
  rqt.tv_nsec = 1000000000;
  assert(do_clock_nanosleep(clk_mono, TIMER_ABSTIME, &rmt, NULL) == EINVAL);
  assert(do_clock_nanosleep(clk_rltm, TIMER_ABSTIME, &rmt, NULL) == EINVAL);
  assert(do_clock_nanosleep(clk_mono, TIMER_ABSTIME, &rmt, &rqt) == EINVAL);
  assert(do_clock_nanosleep(clk_rltm, TIMER_ABSTIME, &rmt, &rqt) == EINVAL);
  assert(do_clock_nanosleep(clk_mono, TIMER_RELTIME, &rmt, NULL) == EINVAL);
  assert(do_clock_nanosleep(clk_rltm, TIMER_RELTIME, &rmt, NULL) == EINVAL);
  assert(do_clock_nanosleep(clk_mono, TIMER_RELTIME, &rmt, &rqt) == EINVAL);
  assert(do_clock_nanosleep(clk_rltm, TIMER_RELTIME, &rmt, &rqt) == EINVAL);

  rqt.tv_nsec = 100000;
  /* Incorrect flags */
  assert(do_clock_nanosleep(clk_mono, ~TIMER_ABSTIME, &rmt, NULL) == EINVAL);
  assert(do_clock_nanosleep(clk_rltm, ~TIMER_ABSTIME, &rmt, NULL) == EINVAL);
  assert(do_clock_nanosleep(clk_mono, ~TIMER_ABSTIME, &rmt, &rqt) == EINVAL);
  assert(do_clock_nanosleep(clk_rltm, ~TIMER_ABSTIME, &rmt, &rqt) == EINVAL);

  return KTEST_SUCCESS;
}

KTEST_ADD(do_clock_nanosleep, test_do_clock_nanosleep, 0);