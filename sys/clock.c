#define KL_LOG KL_TIME
#include <callout.h>
#include <sched.h>
#include <klog.h>
#include <timer.h>
#include <sysinit.h>

static const bintime_t tick = HZ2BT(1000); /* 1ms */
static systime_t ticks = 0;
static timer_t *clock = NULL;

static void clock_cb(timer_t *tm, void *arg) {
  ticks++;
  callout_process(ticks);
  sched_clock();
}

static void clock_init(void) {
  clock = tm_alloc(NULL, TMF_PERIODIC);
  if (clock == NULL)
    panic("Missing suitable timer for maintenance of system clock!");
  tm_init(clock, clock_cb, NULL);
  if (tm_start(clock, TMF_PERIODIC, tick))
    panic("Failed to start system clock!");
  klog("System clock uses \'%s\' hardware timer.", clock->tm_name);
}

SYSINIT_ADD(clock, clock_init, DEPS("sched", "callout", "pit"));
