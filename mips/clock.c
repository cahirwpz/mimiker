#include <common.h>
#include <mips/config.h>
#include <mips/mips.h>
#include <mips/intr.h>
#include <interrupt.h>
#include <bus.h>
#include <callout.h>
#include <mutex.h>
#include <sched.h>
#include <sysinit.h>
#include <mips/clock.h>
#include <timer.h>
#include <klog.h>

timeval_t get_uptime() {
  /* BUG: C0_COUNT will overflow every 4294 seconds for 100MHz processor! */
  uint32_t count = mips32_get_c0(C0_COUNT) / TICKS_PER_US;
  return TIMEVAL_PAIR(count / 1000000, count % 1000000);
}

void mips_clock(timer_event_t *tev) {
  systime_t st = tv2st(tev->tev_when);
  timeval_add(&tev->tev_when, &TIMEVAL(0.001), &tev->tev_when);
  cpu_timer_add_event(tev);
  clock(st);
}
