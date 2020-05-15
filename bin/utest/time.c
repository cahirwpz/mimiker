#include "utest.h"

#include <assert.h>
#include <stdio.h>
#include <sys/time.h>

int test_gettimeofday(void) {
  timeval_t time;
  gettimeofday(&time, NULL);
  printf("Timeofday: %lld seconds and %d microseconds\n", time.tv_sec,
         time.tv_usec);
  /* For XXI century */
  const int64_t start_of_century = 94684800, end_of_century = 4102444799;
  assert(time.tv_sec >= start_of_century);
  assert(time.tv_sec <= end_of_century);
  return 0;
}