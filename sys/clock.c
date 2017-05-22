#include <clock.h>
#include <thread.h>
#include <callout.h>
#include <sched.h>
#include <mips/config.h>

static realtime_t ms;

timeval_t clock_get() {
  uint32_t count = mips32_get_c0(C0_COUNT);
  return TIMEVAL_INIT(count / TICKS_PER_SEC, count % TICKS_PER_SEC);
}

void clock(realtime_t _ms) {
  assert(_ms > ms);
  ms = _ms;
  callout_process(_ms);
  sched_clock();
}
