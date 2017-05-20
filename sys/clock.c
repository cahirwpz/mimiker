#include <clock.h>
#include <thread.h>
#include <callout.h>
#include <sched.h>

static realtime_t ms;

realtime_t clock_get() {
  return ms;
}

void clock(realtime_t _ms) {
  if (ms != _ms) {
    ms = _ms;
    callout_process(_ms);
    sched_clock();
  }
}
