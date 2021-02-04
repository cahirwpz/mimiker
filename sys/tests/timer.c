#include <sys/mimiker.h>
#include <sys/time.h>
#include <sys/klog.h>
#include <sys/ktest.h>

#define N 1000

static int test_timer_monotonic(void) {
  bintime_t prev = BINTIME(0.0);

  for (int i = 0; i < N; i++) {
    bintime_t curr = binuptime();
    assert((curr.sec > prev.sec) ||
           ((curr.sec == prev.sec) && (curr.frac > prev.frac)));
    prev = curr;
  }


  return KTEST_SUCCESS;
}

KTEST_ADD(timer_monotonic, test_timer_monotonic, 0);
