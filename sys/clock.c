#include <time.h>
#include <callout.h>
#include <sched.h>

static realtime_t ms;

void clock(realtime_t _ms) {
  assert(_ms > ms);
  ms = _ms;
  callout_process(_ms);
  sched_clock();
}
