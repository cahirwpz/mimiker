/* Programmable Interval Timer (PIT) driver for Intel 8253 */
#include <sys/mimiker.h>
#include <dev/i8253reg.h>
#include <dev/isareg.h>
#include <sys/pci.h>
#include <sys/interrupt.h>
#include <sys/timer.h>
#include <sys/devclass.h>

typedef struct pit_state {
  resource_t *regs;
  resource_t *irq_res;
  timer_t timer;
  uint16_t period_ticks; /* number of PIT ticks in full period */
  uint32_t prev_ticks;   /* number of ticks since last interrupt */
  uint32_t prev_period;  /* number of periods (in ticks) since last interrupt */
  /* time of last interrupt counted from timer start */
  uint32_t last_irq_ticks; /* [0, TIMER_FREQ) fractional part of a second */
  uint32_t last_irq_sec;   /* seconds */
} pit_state_t;

#define inb(addr) bus_read_1(pit->regs, (addr))
#define outb(addr, val) bus_write_1(pit->regs, (addr), (val))

static void pit_set_frequency(pit_state_t *pit) {
  outb(TIMER_MODE, TIMER_SEL0 | TIMER_16BIT | TIMER_RATEGEN);
  outb(TIMER_CNTR0, pit->period_ticks & 0xff);
  outb(TIMER_CNTR0, pit->period_ticks >> 8);
}

/* Returns value in range [0, pit->period_ticks) */
static uint32_t pit_get_counter(pit_state_t *pit) {
  assert(intr_disabled());

  uint32_t cntr = 0;
  outb(TIMER_MODE, TIMER_SEL0 | TIMER_LATCH);
  cntr |= inb(TIMER_CNTR0);
  cntr |= inb(TIMER_CNTR0) << 8;
  /* PIT counter counts from `period_ticks` to 1 and when it reaches 1
   * an interrupt is triggered and the counter starts from the beginning. */
  return pit->period_ticks - cntr;
}

/* We will not lose counter overflows if the time (in ticks) between
 * consecutive calls to this procedure is less or equal to `period_ticks`. */
static void pit_get_ticks(pit_state_t *pit) {
  uint32_t now = pit_get_counter(pit);

  /* Counter overflow detection. */
  if (pit->prev_ticks > now + pit->prev_period)
    pit->prev_period += pit->period_ticks;

  now += pit->prev_period;

  assert(pit->prev_ticks <= now);

  pit->prev_ticks = now;
}

static intr_filter_t pit_intr(void *data) {
  pit_state_t *pit = data;

  /* XXX: It's still possible for a tick to be lost. */
  pit_get_ticks(pit);

  /* Update the time of last interrupt. */
  pit->last_irq_ticks += pit->prev_period;
  pit->prev_ticks -= pit->prev_period;
  pit->prev_period = 0;

  if (pit->last_irq_ticks >= TIMER_FREQ) {
    pit->last_irq_ticks -= TIMER_FREQ;
    pit->last_irq_sec++;

    assert(pit->last_irq_ticks < TIMER_FREQ);
  }

  tm_trigger(&pit->timer);
  return IF_FILTERED;
}

static device_t *device_of(timer_t *tm) {
  return tm->tm_priv;
}

static int timer_pit_start(timer_t *tm, unsigned flags, const bintime_t start,
                           const bintime_t period) {
  assert(flags & TMF_PERIODIC);
  assert(!(flags & TMF_ONESHOT));

  device_t *dev = device_of(tm);
  pit_state_t *pit = dev->state;

  uint32_t period_ticks = bintime_mul(period, TIMER_FREQ).sec;
  /* Maximal counter value which we can store in pit timer */
  assert(period_ticks <= 0xFFFF);

  pit->prev_ticks = 0;
  pit->prev_period = 0;
  pit->last_irq_sec = 0;
  pit->last_irq_ticks = 0;
  pit->period_ticks = period_ticks;

  pit_set_frequency(pit);

  bus_intr_setup(dev, pit->irq_res, pit_intr, NULL, pit, "i8254 timer");
  return 0;
}

static int timer_pit_stop(timer_t *tm) {
  device_t *dev = device_of(tm);
  pit_state_t *pit = dev->state;
  bus_intr_teardown(dev, pit->irq_res);
  return 0;
}

/* gettime is on HOT PATH, so it must do as little as possible! */
static bintime_t timer_pit_gettime(timer_t *tm) {
  device_t *dev = device_of(tm);
  pit_state_t *pit = dev->state;
  uint32_t ticks, sec;

  WITH_INTR_DISABLED {
    pit_get_ticks(pit);
    ticks = pit->last_irq_ticks + pit->prev_ticks;
    sec = pit->last_irq_sec;
  }

  bintime_t bt = bintime_mul(HZ2BT(TIMER_FREQ), ticks);
  bt.sec += sec;
  return bt;
}

static int pit_attach(device_t *dev) {
  pit_state_t *pit = dev->state;

  pit->regs = device_take_ioports(dev, 0, RF_ACTIVE);
  assert(pit->regs != NULL);

  pit->irq_res = device_take_irq(dev, 0, RF_ACTIVE);

  pit->timer = (timer_t){
    .tm_name = "i8254",
    .tm_flags = TMF_PERIODIC,
    .tm_quality = 100,
    .tm_frequency = TIMER_FREQ,
    .tm_min_period = HZ2BT(TIMER_FREQ),
    .tm_max_period = bintime_mul(HZ2BT(TIMER_FREQ), 65536),
    .tm_start = timer_pit_start,
    .tm_stop = timer_pit_stop,
    .tm_gettime = timer_pit_gettime,
    .tm_priv = dev,
  };

  tm_register(&pit->timer);

  return 0;
}

static int pit_probe(device_t *dev) {
  return dev->unit == 3; /* XXX: unit 3 assigned by gt_pci */
}

static driver_t pit_driver = {
  .desc = "i8254 PIT driver",
  .size = sizeof(pit_state_t),
  .pass = FIRST_PASS,
  .attach = pit_attach,
  .probe = pit_probe,
};

DEVCLASS_ENTRY(pci, pit_driver);
