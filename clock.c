#include "common.h"
#include "clock.h"
#include "interrupts.h"
#include "global_config.h"
#include "pic32mz.h"
#include "libkern.h"

/* This counter is incremented every millisecond. */
static volatile uint32_t timer_ms_count;

void clock_init() {
  uint32_t sr = _mips_intdisable();

  mips32_set_c0(C0_COUNT, 0);
  mips32_set_c0(C0_COMPARE, TICKS_PER_MS);

  timer_ms_count = 0;

  /* Clear core timer interrupt status. */
  IFSSET(0) = 1;
  /* Set core timer interrupts priority to 6 (highest) */
  uint32_t p = IPC(0);
  p &= ~PIC32_IPC_IP0(7); // Clear priority 0 bits
  p |= PIC32_IPC_IP0(6);  // Set them to 6
  IPC(0) = p;
  /* Enable core timer interrupts. */
  IECSET(0) = 1;

  /* It is safe now to re-enable interrupts. */
  _mips_intrestore(sr);
}

uint32_t clock_get_ms() {
  return timer_ms_count;
}

void hardclock() {
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
}
