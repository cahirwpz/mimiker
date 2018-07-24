#include <errno.h>
#include <time.h>

int do_clock_gettime(clockid_t clk, timespec_t *tp) {
  return -ENOTSUP;
}

int do_clock_nanosleep(clockid_t clk, int flags, const timespec_t *rqtp,
                       timespec_t *rmtp) {
  return -ENOTSUP;
}
