#include "utest.h"

#include <stdio.h>
#include <assert.h>
#include <sys/time.h>

int test_gettimeofday(void) {
  timeval_t time1, time2;
  const int64_t start_of_century = 94684800, end_of_century = 4102444799;

  for (int g = 0; g < 100; g++) {
    assert(gettimeofday(&time1, NULL) == 0);
    assert(gettimeofday(&time2, NULL) == 0);
    /* Time can't move backwards */
    assert(timercmp(&time2, &time1, <) == 0);
    /* Time belong to XXI century */
    assert(time1.tv_sec >= start_of_century);
    assert(time1.tv_sec <= end_of_century);
    assert(time2.tv_sec >= start_of_century);
    assert(time2.tv_sec <= end_of_century);
  }

  return 0;
}