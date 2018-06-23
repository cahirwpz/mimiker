#define KL_LOG KL_TIME
#include <callout.h>
#include <sched.h>
#include <klog.h>
#include <timer.h>
#include <sysinit.h>

#define SYSTIME_FREQ 1000 /* 1[tick] = 1[ms] */

static systime_t now = 0;
static timer_t *clock = NULL;

systime_t getsystime(void) {
  return now;
}

static void clock_cb(timer_t *tm, void *arg) {
  now = bintime_mul(getbintime(), SYSTIME_FREQ).sec;
  callout_process(now);
  sched_clock();
}

static void clock_init(void) {
  clock = tm_reserve(NULL, TMF_PERIODIC);
  if (clock == NULL)
    panic("Missing suitable timer for maintenance of system clock!");
  tm_init(clock, clock_cb, NULL);
  if (tm_start(clock, TMF_PERIODIC | TMF_TIMESOURCE, (bintime_t){},
               HZ2BT(SYSTIME_FREQ)))
    panic("Failed to start system clock!");
  klog("System clock uses \'%s\' hardware timer.", clock->tm_name);
}

SYSINIT_ADD(clock, clock_init, DEPS("sched", "callout", "pit"));
