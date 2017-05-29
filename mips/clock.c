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
#include <timer.h>
#include <malloc.h>
#include <sync.h>

timeval_t get_uptime(void) {
  /* BUG: C0_COUNT will overflow every 4294 seconds for 100MHz processor! */
  uint32_t count = mips32_get_c0(C0_COUNT) / TICKS_PER_US;
  return (timeval_t){.tv_sec = count / 1000000, .tv_usec = count % 1000000};
}

static void mips_clock(timer_event_t *tev) {
  systime_t st = tv2st(tev->tev_when);
  tev->tev_when = timeval_add(&tev->tev_when, &TIMEVAL(0.001));
  cpu_timer_add_event(tev);
  clock(st);
}

static void mips_clock_init(void) {
  timer_event_t *clock = kmalloc(M_TEMP, sizeof(*clock), M_ZERO);
  clock->tev_when = TIMEVAL(0.001);
  clock->tev_func = mips_clock;
  CRITICAL_SECTION
    cpu_timer_add_event(clock);
}

SYSINIT_ADD(mips_clock, mips_clock_init, DEPS("cpu_timer"));
