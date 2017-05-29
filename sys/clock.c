#include <callout.h>
#include <sched.h>
#include <timer.h>
#include <sysinit.h>

static timeval_t tick = TIMEVAL(0.001);

static void clock(timer_event_t *tev) {
  systime_t st = tv2st(tev->tev_when);
  tev->tev_when = timeval_add(&tev->tev_when, &tick);
  cpu_timer_add_event(tev);
  callout_process(st);
  sched_clock();
}

static timer_event_t *clock_event = &(timer_event_t){ .tev_func = clock };

static void clock_init(void) {
  timeval_t now = get_uptime();
  clock_event->tev_when = timeval_add(&now, &tick);
  cpu_timer_add_event(clock_event);
}

SYSINIT_ADD(clock, clock_init, DEPS("sched", "callout"));
