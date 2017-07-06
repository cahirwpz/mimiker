#include <sys/time.h>
#include <sys/types.h>
#include <assert.h>

typedef struct test_data {
  time_t sec;
  long nsec;
  long min_us_diff;
} test_data_t;

#define TESTS_NUM 2
static test_data_t td[TESTS_NUM] = {{1, 0, 1000000}, {1, 10, 1010000}};

static int test_sleep(time_t sec, long nsec, long min_us_diff) {
  timespec_t ts = (timespec_t){.tv_sec = sec, .tv_nsec = nsec};
  timeval_t start, end;
  gettimeofday(&start, NULL);
  int result = nanosleep(&ts, NULL);
  gettimeofday(&end, NULL);
  if (result != 0)
    return result;
  timeval_t diff = timeval_sub(&end, &start);
  if (diff.tv_sec * 1000000 + diff.tv_usec < min_us_diff)
    return 1;
  return 0;
}

int test_nanosleep() {
  for (int i = 0; i < TESTS_NUM; i++)
    assert(test_sleep(td[i].sec, td[i].nsec, td[i].min_us_diff) == 0);
  return 0;
}
