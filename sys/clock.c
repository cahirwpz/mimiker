#include <time.h>
#include <callout.h>
#include <sched.h>

static systime_t ms;

void clock(systime_t _ms) {
  assert(_ms > ms);
  ms = _ms;
  callout_process(_ms);
  sched_clock();
}
