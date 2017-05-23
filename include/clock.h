#ifndef _SYS_CLOCK_H_
#define _SYS_CLOCK_H_

#include <time.h>

/* platform specific clock implementation */
timeval_t cpu_clock_get();

static inline timeval_t clock_get() {
  return cpu_clock_get();
}

void clock(realtime_t ms);

#endif /* !_SYS_CLOCK_H_ */
