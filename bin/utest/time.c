#include "utest.h"

#include <stdio.h>
#include <sys/time.h>

int test_gettimeofday(void) {
  timeval_t time;
  gettimeofday(&time, NULL);

  printf("Timeofday: %lld second and %d microsecond\n", time.tv_sec,
         time.tv_usec);
  return 0;
}