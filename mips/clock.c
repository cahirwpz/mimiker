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

/* System clock gets incremented every milisecond. */
static timeval_t sys_clock = TIMEVAL(0);
static timeval_t msec = TIMEVAL(0.001);

timeval_t get_uptime() {
  /* BUG: C0_COUNT will overflow every 4294 seconds for 100MHz processor! */
  uint32_t count = mips32_get_c0(C0_COUNT) / TICKS_PER_US;
  return TIMEVAL_PAIR(count / 1000000, count % 1000000);
}

static void mips_timer_intr() {
  uint32_t compare = mips32_get_c0(C0_COMPARE);
  uint32_t count = mips32_get_c0(C0_COUNT);
  int32_t diff = compare - count;

  /* Should not happen. Potentially spurious interrupt. */
  if (diff > 0)
    return;

  /* This loop is necessary, because sometimes we may miss some ticks. */
  while (diff < TICKS_PER_MS) {
    compare += TICKS_PER_MS;
    timeval_add(&sys_clock, &msec, &sys_clock);
    diff = compare - count;
  }

  /* Set compare register. */
  assert(compare % TICKS_PER_MS == 0);
  mips32_set_c0(C0_COMPARE, compare);
  clock(tv2st(sys_clock));
}

static INTR_HANDLER_DEFINE(mips_timer_intr_handler, NULL, mips_timer_intr, NULL,
                           "MIPS cpu timer", 0);

static void mips_timer_init() {
  mips32_set_c0(C0_COUNT, 0);
  mips32_set_c0(C0_COMPARE, TICKS_PER_MS);
  mips_intr_setup(mips_timer_intr_handler, 7);
}

SYSINIT_ADD(mips_timer, mips_timer_init, DEPS("callout", "sched"));
