#include <common.h>
#include <mips/config.h>
#include <mips/mips.h>
#include <mips/clock.h>
#include <callout.h>
#include <mutex.h>
#include <sched.h>
#include <sysinit.h>

/* This counter is incremented every microsecond. */
static timeval_t mips_clock_us;

static void mips_clock_init() {
  mips32_set_c0(C0_COUNT, 0);
  mips32_set_c0(C0_COMPARE, TICKS_PER_MS);

  timeval_clear(&mips_clock_us);

  /* Enable core timer interrupts. */
  mips32_bs_c0(C0_STATUS, SR_IM7);
}

void mips_clock_irq_handler() {
  uint32_t compare = mips32_get_c0(C0_COMPARE);
  uint32_t count = mips32_get_c0(C0_COUNT);
  int32_t diff = compare - count;

  /* Should not happen. Potentially spurious interrupt. */
  if (diff > 0)
    return;

  timeval_t us = (timeval_t){.tv_sec = 0, .tv_usec = 1000};
  /* This loop is necessary, because sometimes we may miss some ticks. */
  while (diff < TICKS_PER_MS) {
    compare += TICKS_PER_MS;
    timeval_add(&mips_clock_us, &us, &mips_clock_us);
    diff = compare - count;
  }

  /* Set compare register. */
  mips32_set_c0(C0_COMPARE, compare);
  clock(timeval_get_ms(&mips_clock_us));
}

SYSINIT_ADD(mips_clock, mips_clock_init, DEPS("callout", "sched"));
