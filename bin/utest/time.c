#include "utest.h"

#include <assert.h>
#include <sys/errno.h>
#include <sys/time.h>
#include <stdio.h>

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

int test_nanosleep(void) {
  timespec_t treq, trem;
  timeval_t time1, time2, diff;
  int error;

  /* Incorrect arguments */
  treq.tv_sec = -1;
  treq.tv_nsec = 1;
  assert(nanosleep(&treq, NULL) == EINVAL);
  assert(nanosleep(&treq, &trem) == EINVAL);

  treq.tv_sec = 0;
  treq.tv_nsec = -1;
  assert(nanosleep(&treq, NULL) == EINVAL);
  assert(nanosleep(&treq, &trem) == EINVAL);

  treq.tv_nsec = 1000000000;
  assert(nanosleep(&treq, NULL) == EINVAL);
  assert(nanosleep(&treq, &trem) == EINVAL);

  /* Check if sleept at least requested time */;
  
  for (int g = 0; g < 20; g++) {  
    treq.tv_nsec = (1000 << g);
    diff = *((timeval_t *)&treq);
    diff.tv_usec /= 1000;

    assert(gettimeofday(&time1, NULL) == 0);
    while ((error = nanosleep(&treq, &trem)) == EINTR)
      treq = trem;
    assert(error == 0);
    assert(gettimeofday(&time2, NULL) == 0);

    timeradd(&time1, &diff, &time1);
    assert(timercmp(&time1, &time2, <=));
  }
  return 0;
}