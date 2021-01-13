#include "utest.h"

#include <assert.h>
#include <errno.h>
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

#define assert_fail(expr, err) assert(expr == -1 && errno == err)

int test_nanosleep(void) {
  /* Requested and remaining time */
  timespec_t rqt, rmt;
  timeval_t time1, time2, diff;
  int ret;

  /* Incorrect arguments */
  rqt.tv_sec = -1;

  rqt.tv_nsec = 1;
  assert_fail(nanosleep(&rqt, NULL), EINVAL);
  assert_fail(nanosleep(&rqt, &rmt), EINVAL);

  rqt.tv_sec = 0;
  rqt.tv_nsec = -1;
  assert_fail(nanosleep(&rqt, NULL), EINVAL);
  assert_fail(nanosleep(&rqt, &rmt), EINVAL);

  rqt.tv_nsec = 1000000000;
  assert_fail(nanosleep(&rqt, NULL), EINVAL);
  assert_fail(nanosleep(&rqt, &rmt), EINVAL);

  rqt.tv_nsec = 1000;
  assert_fail(nanosleep(NULL, NULL), EFAULT);
  assert_fail(nanosleep(NULL, &rmt), EFAULT);
  assert_fail(nanosleep(0, NULL), EFAULT);
  assert_fail(nanosleep(0, &rmt), EFAULT);

  /* Check if sleept at least requested time */;
  for (int g = 0; g < 20; g++) {
    rqt.tv_nsec = (1000 << g);
    diff = *((timeval_t *)&rqt);
    diff.tv_usec /= 1000;

    ret = gettimeofday(&time1, NULL);
    assert(ret == 0);
    while ((ret = nanosleep(&rqt, &rmt)) == EINTR)
      rqt = rmt;
    assert(ret == 0);
    ret = gettimeofday(&time2, NULL);
    assert(ret == 0);

    timeradd(&time1, &diff, &time1);
    assert(timercmp(&time1, &time2, <=));
  }
  return 0;
}