#include <mips/m32c0.h>
#include <mips/config.h>
#include <mips/intr.h>
#include <interrupt.h>
#include <time.h>
#include <timer.h>

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

typedef struct mips_timer_state {
  bintime_t time;
  bintime_t period;
  uint32_t period_cntr;
  volatile bool overflow;
  volatile counter_t count;
  volatile counter_t compare;
  intr_handler_t intr_handler;
} mips_timer_state_t;

static intr_filter_t mips_timer_intr(void *data);
static int mips_timer_start(timer_t *tm, unsigned flags, const bintime_t start,
                            const bintime_t period);
static int mips_timer_stop(timer_t *tm);
static bintime_t mips_timer_gettime(timer_t *tm);

static timer_t mips_timer;

static mips_timer_state_t mips_timer_state = {
  .intr_handler =
    INTR_HANDLER_INIT(mips_timer_intr, NULL, &mips_timer, "MIPS CPU timer", 0),
};

static timer_t mips_timer = {
  .tm_name = "mips-cpu-timer",
  .tm_flags = TMF_PERIODIC,
  .tm_frequency = CPU_FREQ,
  .tm_min_period = BINTIME(1 / (double)CPU_FREQ),
  .tm_max_period = BINTIME(((1LL << 32) - 1) / (double)CPU_FREQ),
  .tm_start = mips_timer_start,
  .tm_stop = mips_timer_stop,
  .tm_gettime = mips_timer_gettime,
  .tm_priv = &mips_timer_state,
};

static uint64_t read_count(mips_timer_state_t *state) {
  SCOPED_INTR_DISABLED();
  state->count.lo = mips32_get_c0(C0_COUNT);
  return state->count.val;
}

static void set_next_tick(mips_timer_state_t *state) {
  SCOPED_INTR_DISABLED();
  state->compare.val += state->period_cntr;
  mips32_set_c0(C0_COMPARE, state->compare.lo);
}

static mips_timer_state_t *state_of(timer_t *tm) {
  return tm->tm_priv;
}

static intr_filter_t mips_timer_intr(void *data) {
  mips_timer_state_t *state = state_of(data);
  set_next_tick(state);
  tm_trigger(data);
  return IF_FILTERED;
}

static int mips_timer_start(timer_t *tm, unsigned flags, const bintime_t start,
                            const bintime_t period) {
  assert(flags & TMF_PERIODIC);
  assert(!(flags & TMF_ONESHOT));

  mips_timer_state_t *state = state_of(tm);

  state->time = getbintime();
  state->period = period;
  state->period_cntr = bintime_mul(period, CPU_FREQ).sec;
  state->compare.val = read_count(state);
  set_next_tick(state);
  mips_intr_setup(&state->intr_handler, MIPS_HWINT5);
  return 0;
}

static int mips_timer_stop(timer_t *tm) {
  mips_timer_state_t *state = state_of(tm);
  mips_intr_teardown(&state->intr_handler);
  return 0;
}

static bintime_t mips_timer_gettime(timer_t *tm) {
  uint64_t count = read_count(state_of(tm));
  uint32_t sec = count / CPU_FREQ;
  uint32_t frac = count % CPU_FREQ;
  bintime_t bt = bintime_mul(HZ2BT(CPU_FREQ), frac);
  bt.sec = sec;
  return bt;
}

static inline timeval_t ticks2tv(uint64_t ticks) {
  ticks /= (uint64_t)TICKS_PER_US;
  return (timeval_t){.tv_sec = ticks / 1000000LL, .tv_usec = ticks % 1000000LL};
}

timeval_t getcputime(void) {
  return ticks2tv(read_count(state_of(&mips_timer)));
}

void mips_timer_init(void) {
  tm_register(&mips_timer);
  tm_select(&mips_timer);
}
