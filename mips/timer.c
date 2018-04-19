#include <mips/m32c0.h>
#include <mips/config.h>
#include <mips/intr.h>
#include <interrupt.h>
#include <time.h>

/* For now, sole purpose of MIPS CPU timer is to provide accurate timestamps
 * for various parts of the system.
 *
 * TODO Integrate with new timer framework. */

typedef union {
  struct {
    uint32_t lo;
    uint32_t hi;
  };
  uint64_t val;
} counter_t;

static counter_t count = {.hi = 0, .lo = 0};
static counter_t compare = {.hi = 1, .lo = 0};

static uint64_t read_count(void) {
  uint64_t now;
  intr_disable();
  count.lo = mips32_get_c0(C0_COUNT);
  now = count.val;
  intr_enable();
  return now;
}

static intr_filter_t mips_timer_intr(void *data) {
  count.hi++;
  compare.hi++;
  /* To mark interrupt as handled we need to write to compare register! */
  mips32_set_c0(C0_COMPARE, compare.lo);
  return IF_FILTERED;
}

static intr_handler_t mips_timer_intr_handler =
  INTR_HANDLER_INIT(mips_timer_intr, NULL, NULL, "MIPS CPU timer", 0);

static inline timeval_t ticks2tv(uint64_t ticks) {
  ticks /= (uint64_t)TICKS_PER_US;
  return (timeval_t){.tv_sec = ticks / 1000000LL, .tv_usec = ticks % 1000000LL};
}

timeval_t get_uptime(void) {
  return ticks2tv(read_count());
}

void mips_timer_init(void) {
  /* Reset cpu timer. */
  mips32_set_c0(C0_COUNT, 0);
  mips32_set_c0(C0_COMPARE, 0);

  /* Let's permanently enable interrupt handler, as we need to generate
   * interrupt to register counter overflow to correctly maintain time. */
  mips_intr_setup(&mips_timer_intr_handler, MIPS_HWINT5);
}
