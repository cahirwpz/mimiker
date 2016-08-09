#include <common.h>
#include <config.h>
#include <interrupts.h>
#include <mips.h>
#include <clock.h>
#include <callout.h>
#include <libkern.h>

/* This counter is incremented every millisecond. */
static volatile uint32_t timer_ms_count;

void clock_init() {
  intr_disable();

  mips32_set_c0(C0_COUNT, 0);
  mips32_set_c0(C0_COMPARE, TICKS_PER_MS);

  timer_ms_count = 0;

  /* Enable core timer interrupts. */
  mips32_bs_c0(C0_STATUS, SR_IM7);

  /* It is safe now to re-enable interrupts. */
  intr_enable();
}

uint32_t clock_get_ms() {
  return timer_ms_count;
}

void hardclock() {
  log("entering hardclock()");
  uint32_t compare = mips32_get_c0(C0_COMPARE);
  uint32_t count = mips32_get_c0(C0_COUNT);
  int32_t diff = compare - count;

  /* Should not happen. Potentially spurious interrupt. */
  if (diff > 0)
    return;

  /* This loop is necessary, because sometimes we may miss some ticks. */
  while (diff < TICKS_PER_MS) {
    compare += TICKS_PER_MS;
    /* Increment the ms counter. This increment is atomic, because
       entire _interrupt_handler disables nested interrupts. */
    timer_ms_count += 1;
    diff = compare - count;
  }

  /* Set compare register. */
  mips32_set_c0(C0_COMPARE, compare);
  callout_process(0);
  log("leaving hardclock()");
}
